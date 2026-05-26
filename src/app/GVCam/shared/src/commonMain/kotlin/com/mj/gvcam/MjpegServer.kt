package com.mj.gvcam

import kotlinx.coroutines.*
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import java.io.BufferedOutputStream
import java.io.IOException
import java.net.ServerSocket
import java.net.Socket
import java.util.concurrent.CopyOnWriteArrayList
import java.util.concurrent.atomic.AtomicReference

class MjpegServer(private val port: Int = 8080) {

    private val scope = CoroutineScope(Dispatchers.IO + SupervisorJob())
    private var serverSocket: ServerSocket? = null
    private val clients = CopyOnWriteArrayList<Socket>()
    private val clientStreams = mutableMapOf<Socket, BufferedOutputStream>()
    private val latestFrame = AtomicReference<ByteArray?>()
    private val lock = Mutex()
    private var running = false

    var frameCount: Long = 0
        private set
    val clientCount: Int get() = clients.size

    fun start() {
        if (running) return
        running = true

        scope.launch {
            try {
                serverSocket = ServerSocket(port)
                while (running) {
                    val socket = serverSocket?.accept() ?: break
                    handleClient(socket)
                }
            } catch (_: IOException) {}
        }

        scope.launch { sendLoop() }
    }

    private suspend fun handleClient(socket: Socket) {
        withContext(Dispatchers.IO) {
            try {
                socket.tcpNoDelay = true
                socket.sendBufferSize = 512 * 1024
                val out = BufferedOutputStream(socket.getOutputStream(), 256 * 1024)
                val header = "HTTP/1.0 200 OK\r\n" +
                    "Server: GVCam\r\n" +
                    "Connection: close\r\n" +
                    "Max-Age: 0\r\n" +
                    "Expires: 0\r\n" +
                    "Cache-Control: no-cache, private\r\n" +
                    "Pragma: no-cache\r\n" +
                    "Content-Type: multipart/x-mixed-replace; boundary=--boundary\r\n\r\n"
                out.write(header.toByteArray())
                out.flush()
                clients.add(socket)
                clientStreams[socket] = out
            } catch (_: Exception) {
                runCatching { socket.close() }
            }
        }
    }

    fun sendFrame(jpegData: ByteArray) {
        latestFrame.set(jpegData)
    }

    private suspend fun sendLoop() {
        while (running) {
            val data = latestFrame.getAndSet(null)
            if (data != null && clients.isNotEmpty()) {
                broadcast(data)
            }
            delay(1)
        }
    }

    private suspend fun broadcast(jpegData: ByteArray) {
        frameCount++
        val boundary = "--boundary\r\nContent-Type: image/jpeg\r\nContent-Length: ${jpegData.size}\r\n\r\n"
        val boundaryBytes = boundary.toByteArray()
        val footer = "\r\n".toByteArray()
        val packet = boundaryBytes + jpegData + footer

        lock.withLock {
            val it = clients.iterator()
            while (it.hasNext()) {
                val client = it.next()
                try {
                    clientStreams[client]?.let { out ->
                        out.write(packet)
                        out.flush()
                    }
                } catch (_: IOException) {
                    runCatching { client.close() }
                    clientStreams.remove(client)
                    clients.remove(client)
                }
            }
        }
    }

    fun stop() {
        running = false
        for (client in clients) {
            runCatching { client.close() }
        }
        clients.clear()
        clientStreams.clear()
        runCatching { serverSocket?.close() }
        serverSocket = null
        scope.cancel()
    }
}
