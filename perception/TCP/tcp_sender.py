
import socket
import queue
import json
import threading

class TCPSender(threading.Thread):
    RETRY_INTERVAL = 2.0
    SEND_TIMEOUT = 5.0

    def __init__(self, host: str = "127.0.0.1", port: int = 5005,
                 maxsize: int = 10):
        super().__init__(daemon=True, name="TCPSender")
        self.host      = host
        self.port      = port
        self._queue    = queue.Queue(maxsize=maxsize)
        self._stop     = threading.Event()
        self._sock     = None
        self._lock     = threading.Lock()
        self.connected = False


    def send(self, data: dict):
        try:
            self._queue.put_nowait(data)
        except queue.Full:
            try:
                self._queue.get_nowait()
            except queue.Empty:
                pass
            try:
                self._queue.put_nowait(data)
            except queue.Full:
                pass

    def stop(self):
        self._stop.set()
        try:
            self._queue.put_nowait(None)
        except queue.Full:
            pass


    def _connect(self) -> bool:
        with self._lock:
            if self._sock:
                try:
                    self._sock.close()
                except Exception:
                    pass
                self._sock = None
            try:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.settimeout(self.RETRY_INTERVAL)
                s.connect((self.host, self.port))
                s.settimeout(self.SEND_TIMEOUT)
                self._sock     = s
                self.connected = True
                print(f"[TCP] Connected at {self.host}:{self.port}")
                return True
            except (ConnectionRefusedError, OSError):
                self.connected = False
                return False

    def _send_raw(self, payload: str) -> bool:
        with self._lock:
            if self._sock is None:
                return False
            try:
                self._sock.sendall((payload + "\n").encode("utf-8"))
                return True
            except (BrokenPipeError, ConnectionResetError, OSError):
                self.connected = False
                try:
                    self._sock.close()
                except Exception:
                    pass
                self._sock = None
                return False

    def run(self):
        while not self._stop.is_set():

            if not self.connected:
                if not self._connect():
                    self._stop.wait(self.RETRY_INTERVAL)
                    continue

            try:
                data = self._queue.get(timeout=1.0)
            except queue.Empty:
                continue

            if data is None:
                break

            payload = json.dumps(data, separators=(',', ':'))

            if not self._send_raw(payload):
                print("[TCP] Conn. LOST -> reconnect. ...")
                try:
                    self._queue.put_nowait(data)
                except queue.Full:
                    pass

        with self._lock:
            if self._sock:
                try:
                    self._sock.close()
                except Exception:
                    pass
        print("[TCP] Sender STOPPED.")
