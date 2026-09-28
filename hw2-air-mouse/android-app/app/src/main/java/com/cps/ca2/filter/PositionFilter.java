package com.cps.ca2.filter;

import com.cps.ca2.ConstantValues;

public class PositionFilter {
    // Position (meters)
    private float px = 0;
    private float py = 0;
    private float pz = 0;

    // Velocity (m/s)
    private float vx = 0;
    private float vy = 0;
    private float vz = 0;

    // Gravity
    private static final float G = ConstantValues.GRAVITY;

    // Drift control

    public float[] update(
            float[] accel,
            float[] quaternion,
            float dt) {
        float ax = accel[0];
        float ay = accel[1];
        float az = accel[2];

        float[] worldAccel =
                rotateVectorByQuaternion(
                        new float[]{ax, ay, az},
                        quaternion);
        worldAccel[2] -= G;

        vx += worldAccel[0] * dt;
        vy += worldAccel[1] * dt;
        vz += worldAccel[2] * dt;

        px += vx * dt;
        py += vy * dt;
        pz += vz * dt;

        return new float[]{
                px,
                py,
                pz
        };
    }

    private float[] rotateVectorByQuaternion(
            float[] v,
            float[] q) {

        float q0 = q[0];
        float q1 = q[1];
        float q2 = q[2];
        float q3 = q[3];

        float x = v[0];
        float y = v[1];
        float z = v[2];

        float wx =
                2.0f *
                        (
                                (0.5f - q2 * q2 - q3 * q3) * x +
                                        (q1 * q2 - q0 * q3) * y +
                                        (q1 * q3 + q0 * q2) * z
                        );

        float wy =
                2.0f *
                        (
                                (q1 * q2 + q0 * q3) * x +
                                        (0.5f - q1 * q1 - q3 * q3) * y +
                                        (q2 * q3 - q0 * q1) * z
                        );

        float wz =
                2.0f *
                        (
                                (q1 * q3 - q0 * q2) * x +
                                        (q2 * q3 + q0 * q1) * y +
                                        (0.5f - q1 * q1 - q2 * q2) * z
                        );

        return new float[]{
                wx,
                wy,
                wz
        };
    }

    public void reset() {
        px = py = pz = 0;
        vx = vy = vz = 0;
    }

    public float[] getPosition() {
        return new float[]{
                px,
                py,
                pz
        };
    }
}