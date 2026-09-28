package com.cps.ca2.filter;

import com.cps.ca2.ConstantValues;

public class MadgwickFilter {
    private float beta = ConstantValues.MADGWICK_BETA;
    private float dt = ConstantValues.DT;
    // Quaternion state
    private final float[] q = {
            1.0f, 0.0f, 0.0f, 0.0f
    };

    public float[] getEuler() {
        return quaternionToEuler(q.clone());
    }

    public float[] getQuaternion() {
        return q.clone();
    }

    public void update(
            float gx,
            float gy,
            float gz,
            float ax,
            float ay,
            float az,
            float mx,
            float my,
            float mz) {

        float q0 = q[0];
        float q1 = q[1];
        float q2 = q[2];
        float q3 = q[3];

        float norm;
        /*
         * Normalize accelerometer
         */
        norm = sqrt(
                ax * ax +
                        ay * ay +
                        az * az);

        if (norm == 0)
            return;

        ax /= norm;
        ay /= norm;
        az /= norm;

        /*
         * Normalize magnetometer
         */
        norm = sqrt(
                mx * mx +
                        my * my +
                        mz * mz);

        mx /= norm;
        my /= norm;
        mz /= norm;

        float[] gradient =
                new float[4];

        calculateGradientMARG(
                q0, q1, q2, q3,
                ax, ay, az,
                mx, my, mz,
                gradient);

        /*
         * Normalize gradient
         */
        norm = sqrt(
                gradient[0] * gradient[0] +
                        gradient[1] * gradient[1] +
                        gradient[2] * gradient[2] +
                        gradient[3] * gradient[3]);

        if (norm > 0) {
            gradient[0] /= norm;
            gradient[1] /= norm;
            gradient[2] /= norm;
            gradient[3] /= norm;
        }

        /*
         * Quaternion derivative from gyro
         */
        float qDot0 =
                0.5f *
                        (-q1 * gx -
                                q2 * gy -
                                q3 * gz);

        float qDot1 =
                0.5f *
                        (q0 * gx +
                                q2 * gz -
                                q3 * gy);

        float qDot2 =
                0.5f *
                        (q0 * gy -
                                q1 * gz +
                                q3 * gx);

        float qDot3 =
                0.5f *
                        (q0 * gz +
                                q1 * gy -
                                q2 * gx);

        /*
         * Apply feedback
         */
        qDot0 -= beta * gradient[0];
        qDot1 -= beta * gradient[1];
        qDot2 -= beta * gradient[2];
        qDot3 -= beta * gradient[3];

        /*
         * Integrate
         */
        q[0] += qDot0 * dt;
        q[1] += qDot1 * dt;
        q[2] += qDot2 * dt;
        q[3] += qDot3 * dt;

        /*
         * Normalize quaternion
         */
        norm = sqrt(
                q[0] * q[0] +
                        q[1] * q[1] +
                        q[2] * q[2] +
                        q[3] * q[3]);

        if (norm > 0) {
            q[0] /= norm;
            q[1] /= norm;
            q[2] /= norm;
            q[3] /= norm;
        }
    }

    private void calculateGradientIMU(
            float q0,
            float q1,
            float q2,
            float q3,
            float ax,
            float ay,
            float az,
            float[] gradient) {

        float f1 =
                2 * (q1 * q3 - q0 * q2) - ax;

        float f2 =
                2 * (q0 * q1 + q2 * q3) - ay;

        float f3 =
                2 * (0.5f - q1 * q1 - q2 * q2) - az;

        gradient[0] =
                -2 * q2 * f1 +
                        2 * q1 * f2;

        gradient[1] =
                2 * q3 * f1 +
                        2 * q0 * f2 -
                        4 * q1 * f3;

        gradient[2] =
                -2 * q0 * f1 +
                        2 * q3 * f2 -
                        4 * q2 * f3;

        gradient[3] =
                2 * q1 * f1 +
                        2 * q2 * f2;
    }

    private void calculateGradientMARG(
            float q0,
            float q1,
            float q2,
            float q3,
            float ax,
            float ay,
            float az,
            float mx,
            float my,
            float mz,
            float[] gradient) {

        /*
         * This uses the full MARG objective:
         *
         * Gravity + magnetic field
         *
         * The Jacobian form is kept internally.
         */
        float[] f = new float[6];
        float[] J = new float[24];

        float hx =
                2 * (mx * (0.5f - q2 * q2 - q3 * q3)
                        + my * (q1 * q2 - q0 * q3)
                        + mz * (q1 * q3 + q0 * q2));

        float hy =
                2 * (mx * (q1 * q2 + q0 * q3)
                        + my * (0.5f - q1 * q1 - q3 * q3)
                        + mz * (q2 * q3 - q0 * q1));

        float bx = sqrt(hx * hx + hy * hy);

        float bz =
                2 * (mx * (q1 * q3 - q0 * q2)
                        + my * (q2 * q3 + q0 * q1)
                        + mz * (0.5f - q1 * q1 - q2 * q2));

        // Objective functions
        f[0] = 2 * (q1 * q3 - q0 * q2) - ax;
        f[1] = 2 * (q0 * q1 + q2 * q3) - ay;
        f[2] = 2 * (0.5f - q1 * q1 - q2 * q2) - az;

        f[3] = 2 * (bx * (0.5f - q2 * q2 - q3 * q3)
                + bz * (q1 * q3 - q0 * q2)) - mx;

        f[4] = 2 * (bx * (q1 * q2 - q0 * q3)
                + bz * (q0 * q1 + q2 * q3)) - my;

        f[5] = 2 * (bx * (q0 * q2 + q1 * q3)
                + bz * (0.5f - q1 * q1 - q2 * q2)) - mz;

        /*
         * Jacobian
         */
        J[0] = -2 * q2;
        J[1] = 2 * q3;
        J[2] = -2 * q0;
        J[3] = 2 * q1;

        J[4] = 2 * q1;
        J[5] = 2 * q0;
        J[6] = 2 * q3;
        J[7] = 2 * q2;

        J[8] = 0;
        J[9] = -4 * q1;
        J[10] = -4 * q2;
        J[11] = 0;

        // Magnetic part
        J[12] = -2 * bz * q2;
        J[13] = 2 * bx * q3 + 2 * bz * q1;
        J[14] = -4 * bx * q2 + 2 * bz * q0;
        J[15] = -4 * bx * q3 + 2 * bz * q1;

        J[16] = -2 * bx * q3 - 2 * bz * q0;
        J[17] = 2 * bx * q2 - 2 * bz * q1;
        J[18] = 2 * bx * q1 + 2 * bz * q2;
        J[19] = -2 * bx * q0 + 2 * bz * q3;

        J[20] = 2 * bx * q2;
        J[21] = 2 * bx * q3 - 4 * bz * q1;
        J[22] = 2 * bx * q0 - 4 * bz * q2;
        J[23] = 2 * bx * q1;

        for (int i = 0; i < 4; i++)
            gradient[i] = 0;

        for (int row = 0; row < 6; row++) {

            for (int col = 0; col < 4; col++) {

                gradient[col] +=
                        J[row * 4 + col] * f[row];
            }
        }
    }

    private float sqrt(float x) {
        return (float) Math.sqrt(x);
    }

    private float[] quaternionToEuler(float[] q) {
        float q0 = q[0];
        float q1 = q[1];
        float q2 = q[2];
        float q3 = q[3];

        float roll;
        float pitch;
        float yaw;

        // Roll (X axis rotation)
        float sinRoll = 2.0f * (q0 * q1 + q2 * q3);
        float cosRoll = 1.0f - 2.0f * (q1 * q1 + q2 * q2);
        roll = (float)Math.atan2(sinRoll, cosRoll);

        // Pitch (Y axis rotation)
        float sinPitch = 2.0f * (q0 * q2 - q3 * q1);

        // Clamp to avoid NaN from floating point errors
        if (sinPitch > 1.0f)
            sinPitch = 1.0f;
        if (sinPitch < -1.0f)
            sinPitch = -1.0f;
        pitch = (float)Math.asin(sinPitch);

        // Yaw (Z axis rotation)
        float sinYaw = 2.0f * (q0 * q3 + q1 * q2);
        float cosYaw = 1.0f - 2.0f * (q2 * q2 + q3 * q3);
        yaw = (float)Math.atan2(sinYaw, cosYaw);
        // Convert radians to degrees
        return new float[] {
                (float)Math.toDegrees(roll),
                (float)Math.toDegrees(pitch),
                (float)Math.toDegrees(yaw)
        };
    }

    public void reset() {
        q[0] = 1f;
        q[1] = 0f;
        q[2] = 0f;
        q[3] = 0f;
    }
}