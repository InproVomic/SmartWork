"""
OpenClaw 最终工作客户端 - 支持流式响应 + TCP服务端
"""

import asyncio
import json
import time
import os
import sys
import uuid
import socket
import struct

if sys.platform == "win32":
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', line_buffering=True)
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', line_buffering=True)

from cryptography.hazmat.primitives.asymmetric import ed25519
import base64
import hashlib
import websockets


TCP_HOST = '127.0.0.1'
TCP_PORT = 18790


class OpenClawClient:
    """OpenClaw客户端 - 支持流式聊天"""

    def __init__(self, url, token, session_key="agent:main:main"):
        self.url = url
        self.token = token
        self.session_key = session_key
        self.ws = None
        self.message_queue = asyncio.Queue()
        self.connected = False
        self.current_chat_id = None

        self.private_key = ed25519.Ed25519PrivateKey.generate()
        self.device_id = hashlib.sha256(
            self.private_key.public_key().public_bytes_raw()
        ).hexdigest()
        self.public_key_b64 = base64.urlsafe_b64encode(
            self.private_key.public_key().public_bytes_raw()
        ).decode("utf-8").rstrip("=")
        self.connect_nonce = None

    def sign_payload(self, payload):
        sig_bytes = self.private_key.sign(payload.encode("utf-8"))
        return base64.urlsafe_b64encode(sig_bytes).decode("utf-8").rstrip("=")

    async def connect(self):
        """连接到Gateway"""
        self.ws = await websockets.connect(self.url, origin="http://127.0.0.1:18789")

        asyncio.create_task(self._receive_messages())

        await self._do_connect()

        await self._wait_hello()

        self.connected = True

    async def _receive_messages(self):
        """接收消息"""
        try:
            async for message in self.ws:
                try:
                    data = json.loads(message)
                    await self._handle_message(data)
                except json.JSONDecodeError:
                    pass
        except websockets.exceptions.ConnectionClosed:
            self.connected = False

    async def _handle_message(self, data):
        """处理消息"""
        msg_type = data.get("type")
        event = data.get("event")

        if msg_type == "event" and event == "connect.challenge":
            payload = data.get("payload", {})
            self.connect_nonce = payload.get("nonce")

        elif msg_type == "res" and data.get("ok"):
            payload = data.get("payload", {})
            if payload.get("type") == "hello-ok":
                await self.message_queue.put(("hello", None))

        elif msg_type == "event" and event == "chat":
            payload = data.get("payload", {})
            state = payload.get("state", "")
            message = payload.get("message", {})
            content = message.get("content", "")

            if state == "delta" and content:
                if isinstance(content, list) and len(content) > 0:
                    text = content[0].get("text", "")
                    if text:
                        print(f"[OpenClaw] chat_id={self.current_chat_id}, delta: '{text[:30]}...' (len={len(text)})", flush=True)
                        await self.message_queue.put(("delta", text))
            elif state == "final":
                print(f"[OpenClaw] chat_id={self.current_chat_id}, final event received", flush=True)
                await self.message_queue.put(("done", None))
            elif state == "error":
                error_msg = payload.get("error", {}).get("message", "Unknown error")
                print(f"[OpenClaw] chat_id={self.current_chat_id}, error: {error_msg}", flush=True)
                await self.message_queue.put(("error", error_msg))
            else:
                print(f"[OpenClaw] chat_id={self.current_chat_id}, state={state}, content type={type(content)}", flush=True)

        elif msg_type == "event" and event == "chat.error":
            error = data.get("payload", {}).get("error", "Unknown")
            print(f"[OpenClaw] chat.error event: {error}", flush=True)
            await self.message_queue.put(("error", error))

    async def _do_connect(self):
        """发送connect请求"""
        for _ in range(50):
            if self.connect_nonce:
                break
            await asyncio.sleep(0.1)

        if not self.connect_nonce:
            raise ConnectionError("未收到connect.challenge")

        nonce = self.connect_nonce
        signed_at = int(time.time() * 1000)

        payload = (
            f"v2|{self.device_id}|webchat-ui|"
            f"webchat|operator|"
            f"operator.admin,operator.approvals,operator.pairing|"
            f"{signed_at}|{self.token}|{nonce}"
        )

        signature = self.sign_payload(payload)

        connect_req = {
            "type": "req",
            "id": f"connect-{int(time.time()*1000)}",
            "method": "connect",
            "params": {
                "minProtocol": 3,
                "maxProtocol": 3,
                "client": {
                    "id": "webchat-ui",
                    "version": "dev",
                    "platform": "browser",
                    "mode": "webchat"
                },
                "role": "operator",
                "scopes": ["operator.admin", "operator.approvals", "operator.pairing"],
                "auth": {"token": self.token},
                "device": {
                    "id": self.device_id,
                    "publicKey": self.public_key_b64,
                    "signature": signature,
                    "signedAt": signed_at,
                    "nonce": nonce
                },
                "locale": "zh-CN",
                "userAgent": "openclaw-python-client/1.0.0"
            }
        }

        await self.ws.send(json.dumps(connect_req))

    async def _wait_hello(self):
        """等待hello-ok"""
        for _ in range(50):
            try:
                event_type, data = await asyncio.wait_for(
                    self.message_queue.get(), timeout=0.2
                )
                if event_type == "hello":
                    return
            except asyncio.TimeoutError:
                continue

    async def send_message(self, content):
        """发送消息并流式返回响应"""
        chat_id = f"chat-{int(time.time()*1000)}"
        self.current_chat_id = chat_id
        
        chat_req = {
            "type": "req",
            "id": chat_id,
            "method": "chat.send",
            "params": {
                "sessionKey": self.session_key,
                "message": content,
                "idempotencyKey": str(uuid.uuid4())
            }
        }

        print(f"[send_message] chat_id={chat_id}, sending message: '{content[:30]}...'", flush=True)
        
        while not self.message_queue.empty():
            try:
                self.message_queue.get_nowait()
            except asyncio.QueueEmpty:
                break
        print(f"[send_message] queue cleared", flush=True)
        
        await self.ws.send(json.dumps(chat_req))
        
        chunk_count = 0
        last_full_response = ""

        while True:
            try:
                event_type, data = await asyncio.wait_for(
                    self.message_queue.get(), timeout=120
                )

                if event_type == "delta":
                    chunk_count += 1
                    last_full_response = data
                    print(f"[send_message] chat_id={chat_id}, delta #{chunk_count}, len={len(data)}, content='{data[:50]}...'", flush=True)
                elif event_type == "done":
                    print(f"[send_message] chat_id={chat_id}, done, total deltas={chunk_count}, final length={len(last_full_response)}", flush=True)
                    break
                elif event_type == "error":
                    raise Exception(f"Chat error: {data}")

            except asyncio.TimeoutError:
                print(f"[send_message] chat_id={chat_id}, timeout after 120s", flush=True)
                break
        
        print(f"[send_message] chat_id={chat_id}, final response: '{last_full_response[:100]}...'", flush=True)
        
        self.current_chat_id = None
        yield last_full_response

    async def close(self):
        """关闭连接"""
        if self.ws:
            await self.ws.close()
            self.connected = False


class TcpServer:
    """TCP服务端 - 与QT客户端通信"""

    def __init__(self, host, port, openclaw_client):
        self.host = host
        self.port = port
        self.openclaw_client = openclaw_client
        self.server = None
        self.running = False

    async def start(self):
        """启动TCP服务"""
        self.server = await asyncio.start_server(
            self._handle_client, self.host, self.port
        )
        self.running = True
        print(f"[TCP] 服务端启动在 {self.host}:{self.port}", flush=True)

    async def stop(self):
        """停止TCP服务"""
        self.running = False
        if self.server:
            self.server.close()
            await self.server.wait_closed()

    async def _handle_client(self, reader, writer):
        """处理客户端连接"""
        addr = writer.get_extra_info('peername')
        print(f"[TCP] 客户端连接: {addr}", flush=True)

        try:
            while self.running:
                length_data = await reader.readexactly(4)
                length = struct.unpack('>I', length_data)[0]

                if length > 1024 * 1024:
                    print(f"[TCP] 消息过长: {length}", flush=True)
                    break

                data = await reader.readexactly(length)
                message = data.decode('utf-8')

                print(f"[TCP] 收到消息: {message[:50]}...", flush=True)

                try:
                    response = await self._process_message(message)
                    await self._send_response(writer, response)
                except Exception as e:
                    error_response = json.dumps({
                        "success": False,
                        "error": str(e)
                    })
                    await self._send_response(writer, error_response)

        except asyncio.IncompleteReadError:
            print(f"[TCP] 客户端断开: {addr}", flush=True)
        except Exception as e:
            print(f"[TCP] 错误: {e}", flush=True)
        finally:
            writer.close()
            await writer.wait_closed()

    async def _process_message(self, message):
        """处理消息并返回响应"""
        import time
        request_time = time.strftime("%H:%M:%S", time.localtime())
        print(f"[TCP] [{request_time}] Processing message: '{message[:30]}...'", flush=True)
        
        full_response = ""
        response_count = 0

        try:
            async for response in self.openclaw_client.send_message(message):
                response_count += 1
                full_response = response
                print(f"[TCP] [{request_time}] Got response #{response_count}, length: {len(full_response)}", flush=True)
        except Exception as e:
            print(f"[TCP] [{request_time}] Error: {e}", flush=True)
            return json.dumps({
                "success": False,
                "error": str(e)
            })

        print(f"[TCP] [{request_time}] Response complete, length: {len(full_response)}", flush=True)
        print(f"[TCP] [{request_time}] Response preview: '{full_response[:100]}...'", flush=True)
        
        result = json.dumps({
            "success": True,
            "content": full_response
        }, ensure_ascii=False)
        
        print(f"[TCP] [{request_time}] JSON result length: {len(result)}", flush=True)
        return result

    async def _send_response(self, writer, response):
        """发送响应"""
        import time
        send_time = time.strftime("%H:%M:%S", time.localtime())
        
        data = response.encode('utf-8')
        length = len(data)
        
        print(f"[TCP] [{send_time}] Sending response, length: {length}", flush=True)
        
        writer.write(struct.pack('>I', length))
        writer.write(data)
        await writer.drain()
        
        print(f"[TCP] [{send_time}] Response sent successfully", flush=True)


async def main():
    """主函数"""
    URL = "ws://127.0.0.1:18789/socket"
    TOKEN = "938004e0916542433c9308746a9e3707d0310992fedcef73"

    print("="*60, flush=True)
    print("OpenClaw Python 客户端 (TCP服务端模式)", flush=True)
    print("="*60, flush=True)

    client = OpenClawClient(URL, TOKEN)

    print("\n正在连接OpenClaw...", flush=True)
    await client.connect()
    print("[OK] OpenClaw连接成功！", flush=True)

    tcp_server = TcpServer(TCP_HOST, TCP_PORT, client)
    await tcp_server.start()

    print("\n" + "="*60, flush=True)
    print(f"TCP服务端已启动，等待QT客户端连接...", flush=True)
    print(f"地址: {TCP_HOST}:{TCP_PORT}", flush=True)
    print("="*60 + "\n", flush=True)

    try:
        while tcp_server.running:
            await asyncio.sleep(1)
    except KeyboardInterrupt:
        print("\n正在关闭...", flush=True)
    finally:
        await tcp_server.stop()
        await client.close()
        print("已关闭", flush=True)


if __name__ == "__main__":
    asyncio.run(main())
