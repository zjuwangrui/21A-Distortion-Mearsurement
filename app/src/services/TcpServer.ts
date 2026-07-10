/**
 * TcpServer.ts — 手机端 TCP 服务器
 *
 * 作用：监听 :8080，接收 STM32 (通过 ESP8266) POST 过来的 THD 数据。
 *
 * STM32 请求（每 1s 一次）：
 *   POST /thd HTTP/1.0
 *   Content-Length: ...
 *
 *   {"f0":1000.50,"thd":0.324,
 *    "h":[1.0000,0.0324,0.0152,0.0056,0.0031],
 *    "p":[0.00,1.57,3.14,-1.57,0.00]}
 *
 * 手机响应：
 *   HTTP/1.0 200 OK
 *   Content-Length: 2
 *   Connection: close
 *
 *   {}
 *
 * 手机不下发任何控制参数；只做被动接收展示。
 */

import TcpSocket from 'react-native-tcp-socket';

export type ThdData = {
  f0: number;
  thd: number;
  h: number[];   // 5 个：H1..H5 幅度（未归一化）
  p: number[];   // 5 个：H1..H5 相位（弧度）
};

export type ServerCallbacks = {
  onData: (data: ThdData) => void;
  onLog: (msg: string) => void;
  onClientConnect: (address: string) => void;
  onClientDisconnect: () => void;
};

// Singleton server instance
let server: ReturnType<typeof TcpSocket.createServer> | null = null;

// Per-socket receive buffer (handles TCP fragmentation)
const socketBuffers = new Map<number, string>();
let socketIdCounter = 0;

function parseHttpBody(raw: string): string | null {
  const headerEnd = raw.indexOf('\r\n\r\n');
  if (headerEnd === -1) return null;

  const headers = raw.substring(0, headerEnd);
  const match = headers.match(/content-length:\s*(\d+)/i);
  if (!match) return null;

  const contentLength = parseInt(match[1], 10);
  const bodyStart = headerEnd + 4;
  if (raw.length - bodyStart < contentLength) return null;

  return raw.substring(bodyStart, bodyStart + contentLength);
}

function buildHttpResponse(body: string): string {
  return [
    'HTTP/1.0 200 OK',
    'Content-Type: application/json',
    `Content-Length: ${body.length}`,
    'Connection: close',
    '',
    body,
  ].join('\r\n');
}

/** 校验并归一化 THD JSON 到强类型 */
function parseThdBody(body: string): ThdData | null {
  let obj: any;
  try { obj = JSON.parse(body); }
  catch { return null; }

  const isNum  = (x: any) => typeof x === 'number' && !isNaN(x);
  const isArr5 = (x: any) => Array.isArray(x) && x.length === 5 && x.every(isNum);

  if (!isNum(obj.f0) || !isNum(obj.thd) || !isArr5(obj.h) || !isArr5(obj.p)) {
    return null;
  }
  return { f0: obj.f0, thd: obj.thd, h: obj.h, p: obj.p };
}

export function startServer(
  port: number,
  callbacks: ServerCallbacks,
): void {
  if (server) return;

  server = TcpSocket.createServer(socket => {
    const socketId = socketIdCounter++;
    socketBuffers.set(socketId, '');

    const remoteAddr = socket.remoteAddress ?? 'unknown';
    callbacks.onLog(`STM32 已连接: ${remoteAddr}`);
    callbacks.onClientConnect(remoteAddr);

    socket.on('data', (data: Buffer | string) => {
      const chunk = typeof data === 'string' ? data : data.toString('utf8');
      const prev = socketBuffers.get(socketId) ?? '';
      const accumulated = prev + chunk;
      socketBuffers.set(socketId, accumulated);

      const body = parseHttpBody(accumulated);
      if (body === null) return;

      socketBuffers.set(socketId, '');

      const parsed = parseThdBody(body);
      if (!parsed) {
        callbacks.onLog(`JSON 解析失败：${body.substring(0, 80)}`);
        socket.write('HTTP/1.0 400 Bad Request\r\nContent-Length: 0\r\n\r\n');
        socket.destroy();
        return;
      }

      callbacks.onData(parsed);

      // 手机不下发任何控制参数，回空 {} 即可
      socket.write(buildHttpResponse('{}'));
      socket.destroy();
    });

    socket.on('error', err => {
      callbacks.onLog(`Socket 错误: ${err.message}`);
    });

    socket.on('close', () => {
      socketBuffers.delete(socketId);
      callbacks.onClientDisconnect();
    });
  });

  server.listen({port, host: '0.0.0.0'}, () => {
    callbacks.onLog(`✓ TCP 服务器启动，监听 0.0.0.0:${port}`);
  });

  server.on('error', (err: Error) => {
    callbacks.onLog(`服务器错误: ${err.message}`);
    server = null;
  });
}

export function stopServer(): void {
  if (server) {
    server.close();
    server = null;
    socketBuffers.clear();
  }
}

export function isServerRunning(): boolean {
  return server !== null;
}
