package com.cps.ca2;

import static java.lang.Math.abs;
import static java.lang.Math.max;
import static java.lang.Math.min;

import android.app.Activity;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.view.Gravity;
import android.widget.TextView;

import com.cps.ca2.filter.MadgwickFilter;
import com.cps.ca2.filter.PositionFilter;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SensorHandler implements SensorEventListener {

    private final SensorManager sensorManager;
    private final MainActivity activity;
    private float[] orientation = {1f, 0f, 0f, 0f};
    private final float GRAVITY = ConstantValues.GRAVITY;

    private float[] gyroSamples = new float[]{0,0,0};
    private float[] gyroBias = new float[]{-0.0017580127f, 0.0012243048f, 0.0015468756f};
    private float[] magMin, magMax, magOffset=new float[]{-53.746876f, -77.54063f, 26.98125f},
                                    magScale=new float[]{9.796875f, 26.634375f, 8.775001f};
    private float[][] accelSamples = new float[6][3];
    private float[] accelOffset = new float[]{-0.021988899f, 0.2021718f, 0.53474176f},
                    accelScale = new float[]{1.000408f, 0.9914451f, 0.9917036f};
    private int gyroBiasSamplesCount = 0;
    private int[] accelSamplesCount = new int[]{0,0,0,0,0,0};
    private final MadgwickFilter orientationFilter = new MadgwickFilter();
    private final PositionFilter positionFilter = new PositionFilter();
    private final Map.Entry<Integer, Integer>[] EXPECTED_ACCEL_AXIS =  new Map.Entry[] {
            Map.entry(2, +1),
            Map.entry(2, -1),
            Map.entry(0, +1),
            Map.entry(0, -1),
            Map.entry(1, -1),
            Map.entry(1, +1)
    };

    private final Map<String, float[]> data = Map.of(
            "accel", new float[3],
            "gyro", new float[3],
            "mag", new float[3]
    );

    public SensorHandler(MainActivity activity) {
        this.activity = activity;
        this.sensorManager = (SensorManager) activity.getSystemService(Activity.SENSOR_SERVICE);
    }

    public void register() {
        if (sensorManager != null) {
            sensorManager.registerListener(this, sensorManager.getDefaultSensor(
                    Sensor.TYPE_ACCELEROMETER_UNCALIBRATED), SensorManager.SENSOR_DELAY_NORMAL);
            sensorManager.registerListener(this, sensorManager.getDefaultSensor(
                    Sensor.TYPE_GYROSCOPE_UNCALIBRATED), SensorManager.SENSOR_DELAY_NORMAL);
            sensorManager.registerListener(this, sensorManager.getDefaultSensor(
                    Sensor.TYPE_MAGNETIC_FIELD_UNCALIBRATED), SensorManager.SENSOR_DELAY_NORMAL);
        }
    }

    public void unregister() {
        if (sensorManager != null) {
            sensorManager.unregisterListener(this);
        }
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        switch (event.sensor.getType()) {
            case Sensor.TYPE_ACCELEROMETER_UNCALIBRATED:
                System.arraycopy(event.values, 0, data.get("accel"), 0, 3);
                break;
            case Sensor.TYPE_GYROSCOPE_UNCALIBRATED:
                System.arraycopy(event.values, 0, data.get("gyro"), 0, 3);
                break;
            case Sensor.TYPE_MAGNETIC_FIELD_UNCALIBRATED:
                System.arraycopy(event.values, 0, data.get("mag"), 0, 3);
                break;
        }
        activity.printSensorData(data);
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

    public void resetCalibrationData(String type, int accelPosition) {
        switch (type) {
            case "Gyro":
                gyroBias = new float[]{0,0,0};
                gyroBiasSamplesCount = 0;
                break;
            case "Accel":
                accelSamples[accelPosition] = new float[]{0f,0f,0f};
                accelSamplesCount[accelPosition] = 0;
                break;
            case "Mag":
                magMin = new float[]{Float.MAX_VALUE,Float.MAX_VALUE,Float.MAX_VALUE};
                magMax = new float[]{-Float.MAX_VALUE,-Float.MAX_VALUE,-Float.MAX_VALUE};
                break;
        }
    }

    public void sampleGyro() {
        float[] gyro = data.get("gyro");
        gyroBiasSamplesCount++;
        for (int i = 0; i < 3; i++)
            gyroSamples[i] += (gyro[i] - gyroSamples[i]) / gyroBiasSamplesCount;
    }

    public void sampleAccel(int position) {
        float[] accel = data.get("accel");
        accelSamplesCount[position]++;
        for (int i = 0; i < 3; i++)
            accelSamples[position][i] += (accel[i] - accelSamples[position][i]) / accelSamplesCount[position];
    }

    public void sampleMag() {
        float[] mag = data.get("mag");
        for (int i = 0; i < 3; i++) {
            magMin[i] = min(magMin[i], mag[i]);
            magMax[i] = max(magMax[i], mag[i]);
        }
    }

    public void calibrateGyro() {
        gyroBias = gyroSamples.clone();
    }

    public void calibrateAccel() {
        int[] posIdx = new int[3], negIdx = new int[3];
        int idx = 0;
        for (Map.Entry<Integer, Integer> entry : EXPECTED_ACCEL_AXIS) {
            Integer axis = entry.getKey();
            Integer sign = entry.getValue();
            if (sign == 1)
                posIdx[axis] = idx;
            else
                negIdx[axis] = idx;
            idx++;
        }
        for (int i = 0; i < 3; i++) {
            accelOffset[i] = 0.0f;
            for (int j = 0; j < 6; j++)
                accelOffset[i] += accelSamples[j][i] / 2f;

            accelScale[i] = 0.0f;
            accelScale[i] = (accelSamples[posIdx[i]][i] - accelSamples[negIdx[i]][i]) / (2f * GRAVITY);
        }
    }

    public void calibrateMag() {
        for (int i = 0; i < 3; i++) {
            magOffset[i] = (magMax[i] + magMin[i]) / 2;
            magScale[i] = (magMax[i] - magMin[i]) / 2;
        }
    }

    public Map<String, float[]> getCalibratedData() {
        float[] accel = new float[3], gyro = new float[3], mag = new float[3];
        Map<String, float[]> result = new HashMap<>(data);

        for (int i = 0; i < 3; i++) {
            accel[i] = (data.get("accel")[i] - accelOffset[i]) / accelScale[i];
            gyro[i] = data.get("gyro")[i] - gyroBias[i];
            mag[i] = (data.get("mag")[i] - magOffset[i]) / magScale[i];
        }
        result.put("gyro", gyro);
        result.put("accel", accel);
        result.put("mag", mag);
        return result;
    }
}
