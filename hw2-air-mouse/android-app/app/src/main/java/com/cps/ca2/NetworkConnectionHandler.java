package com.cps.ca2;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class NetworkConnectionHandler {
    private DatagramSocket socket;
    private InetAddress address;
    private int port;
    private final int timeoutMs = 1000;
    private final float EPSILON = 0.0001f;
    private ExecutorService executorService;

    public NetworkConnectionHandler() throws SocketException {
        socket = new DatagramSocket();
        socket.setSoTimeout(timeoutMs);
        executorService = Executors.newSingleThreadExecutor();
    }

    public void setAddress(String address, int port) {
        try {
            this.address = InetAddress.getByName(address);
            this.port = port;
        } catch (UnknownHostException e) {
            System.out.println("Invalid address: " + e.getMessage());
        }
    }

    private void sendPacket(byte[] buf, boolean reliabilityRequired) throws IOException {
        DatagramPacket packet = new DatagramPacket(buf, buf.length, address, port);
        socket.send(packet);
        if (!reliabilityRequired)
            return;
        packet = new DatagramPacket(buf, buf.length);
        boolean acked = false;
        while (true)
            try {
                byte[] receiveBuffer = new byte[256];
                DatagramPacket receivePacket = new DatagramPacket(receiveBuffer, receiveBuffer.length);
                socket.receive(receivePacket);
                System.out.println("ACKED!");
                return;
            } catch (SocketTimeoutException e) {
                socket.send(packet);
            }
    }

    public void sendDataAsync(final float moveX, final float moveY,
                              final boolean clickL, final boolean clickR,
                              final float scroll) {
        executorService.submit(new Runnable() {
            @Override
            public void run() {
                try {
                    sendData(moveX, moveY, clickL, clickR, scroll);
                } catch (IOException e) {
                    System.out.println("Send failed: " + e.getMessage());
                }
            }
        });
    }

    private void sendData(float moveX, float moveY, boolean clickL, boolean clickR, float scroll) throws IOException {
        boolean hasMovement = Math.abs(moveX) > EPSILON || Math.abs(moveY) > EPSILON;
        boolean hasScroll = Math.abs(scroll) > EPSILON;

        if (hasMovement) {
            String msg = String.format("M:%.4f,%.4f", moveX, moveY);
            sendPacket(msg.getBytes(), false);
        }

        if (hasScroll) {
            String msg = String.format("S:%.4f", scroll);
            sendPacket(msg.getBytes(), true);
        }

        if (clickL) {
            String msg = "C:L";
            sendPacket(msg.getBytes(), true);
        }

        if (clickR) {
            String msg = "C:R";
            sendPacket(msg.getBytes(), true);
        }
    }
}
