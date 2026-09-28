package com.cps.ca2;

import android.content.res.ColorStateList;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import java.io.IOException;
import java.net.SocketException;
import java.util.Map;

public class MainActivity extends AppCompatActivity {
    private enum AppState {
        IDLE,
        RUNNING,
        CALIBRATING,
    }
    private enum CalibrationStep {
        Gyro,
        Accel,
        Mag,
        None
    }

    private final int TOTAL_WAITING_TIME = ConstantValues.TOTAL_WAITING_TIME,
            TOTAL_CALIB_TIME = ConstantValues.TOTAL_CALIB_TIME,
            TIME_INTERVAL = ConstantValues.TIME_INTERVAL;

    private AppState currentState = AppState.IDLE;
    private CalibrationStep calibrationStep = CalibrationStep.None;
    private int calibrationPosition = 0;
    private Button startButton, calibrateButton;
    private TextView textViewSensor, textViewTimer, textViewInstruction;
    private EditText addressText;
    private ImageView instructionImageView;
    private CountDownTimer calibrationTimer;

    private SensorHandler sensorHandler;
    private NetworkConnectionHandler connHanler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);

        sensorHandler = new SensorHandler(this);
        try {
            connHanler = new NetworkConnectionHandler();
        } catch (SocketException e) {
            throw new RuntimeException(e);
        }
        setContentView(R.layout.activity_main);
        setupWindowInsets();
        transitionToState(AppState.IDLE);
    }

    @Override
    protected void onResume() {
        super.onResume();
        sensorHandler.register();
    }

    @Override
    protected void onPause() {
        super.onPause();
        sensorHandler.unregister();
    }

    private void setupWindowInsets() {
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });
    }

    private void transitionToState(AppState newState) {
        if (newState == AppState.CALIBRATING) {
            setContentView(R.layout.calibration);
            setupWindowInsets();
        } else {
            if (calibrationTimer != null)
                calibrationTimer.cancel();
            setContentView(R.layout.activity_main);
            setupWindowInsets();
        }

        startButton = findViewById(R.id.startButton);
        calibrateButton = findViewById(R.id.calibrateButton);
        textViewSensor = findViewById(R.id.textViewSensor);
        textViewTimer = findViewById(R.id.textViewTimer);
        textViewInstruction = findViewById(R.id.textViewInstruction);
        addressText = findViewById(R.id.addressText);
        instructionImageView = findViewById(R.id.instructionImageView);

        currentState = newState;
        switch (newState) {
            case IDLE:
                handleIdle();
                break;
            case RUNNING:
                handleRunning();
                break;
            case CALIBRATING:
                handleCalibrating();
                break;
        }
    }

    private void handleIdle() {
        startButton.setText("Start");
        startButton.setBackgroundTintList(ColorStateList.valueOf(ContextCompat.getColor(this, R.color.app_green)));
        startButton.setOnClickListener(v -> transitionToState(AppState.RUNNING));

        calibrateButton.setText("Calibrate");
        calibrateButton.setBackgroundTintList(ColorStateList.valueOf(ContextCompat.getColor(this, R.color.app_blue)));
        calibrateButton.setOnClickListener(v -> transitionToState(AppState.CALIBRATING));
    }

    private void handleRunning() {
        startButton.setText("Stop");
        startButton.setBackgroundTintList(ColorStateList.valueOf(ContextCompat.getColor(this, R.color.app_red)));
        startButton.setOnClickListener(v -> transitionToState(AppState.IDLE));

        calibrateButton.setText("Calibrate");
        calibrateButton.setBackgroundTintList(ColorStateList.valueOf(ContextCompat.getColor(this, R.color.app_blue)));
        calibrateButton.setOnClickListener(v -> transitionToState(AppState.CALIBRATING));

        String address = addressText.getText().toString();
        if (address.isEmpty())
            return;
        String ip = address.split(":")[0];
        int port = Integer.parseInt(address.split(":")[1]);
        connHanler.setAddress(ip, port);
    }

    private void handleCalibrating() {
        startButton.setText("Back");
        startButton.setBackgroundTintList(ColorStateList.valueOf(ContextCompat.getColor(this, R.color.app_red)));
        startButton.setOnClickListener(v -> transitionToState(AppState.IDLE));

        calibrateButton.setText("Skip");
        calibrateButton.setBackgroundTintList(ColorStateList.valueOf(ContextCompat.getColor(this, R.color.app_blue)));
        calibrateButton.setOnClickListener(v -> {
            if (calibrationTimer != null) {
                calibrationTimer.cancel();
            }
            doCalibration();
        });

        doCalibration();
    }

    private void nextCalibrationStep() {
        String[] positions = {
                "Face up", "Face down", "Side left", "Side right", "Top edge down", "Bottom edge down"
        };
        int[] images = new int[]{
                R.drawable.up1, R.drawable.down1,
                R.drawable.left1, R.drawable.right1,
                R.drawable.back1, R.drawable.front1
        };
        switch (calibrationStep) {
            case Gyro:
                calibrationPosition = 0;
                calibrationStep = CalibrationStep.Accel;
                textViewInstruction.setText("Place phone: " + positions[calibrationPosition]);
                instructionImageView.setImageResource(images[calibrationPosition]);
                break;
            case Accel:
                calibrationPosition++;
                if (calibrationPosition==6) {
                    calibrationStep = CalibrationStep.Mag;
                    textViewInstruction.setText("Move phone in figure-8 pattern");
                    instructionImageView.setImageResource(R.drawable.mag);
                }
                else {
                    textViewInstruction.setText("Place phone: " + positions[calibrationPosition]);
                    instructionImageView.setImageResource(images[calibrationPosition]);
                }
                break;
            case Mag:
                calibrationStep = CalibrationStep.None;
                transitionToState(AppState.IDLE);
                return;
            case None:
                calibrationStep = CalibrationStep.Gyro;
                textViewInstruction.setText("Keep phone completely still on a flat surface");
                instructionImageView.setImageResource(R.drawable.up1);
                break;
        }
    }

    private void doCalibration() {
        nextCalibrationStep();
        if (currentState != AppState.CALIBRATING)
            return;
        playBeep(true);
        calibrationTimer = new CountDownTimer(TOTAL_WAITING_TIME, TIME_INTERVAL) {
            @Override
            public void onTick(long millisUntilFinished) {
                float secondsRemaining = (float) millisUntilFinished / 1000;
                String timerString = "";
                switch (calibrationStep) {
                    case Gyro:
                        timerString = "Gyro Calibration";
                        break;
                    case Accel:
                        timerString = "Accel Calibration " + (calibrationPosition + 1);
                        break;
                    case Mag:
                        timerString = "Mag Calibration";
                        break;
                }
                textViewTimer.setText(String.format(timerString + " in: %.1fs", secondsRemaining));
            }

            @Override
            public void onFinish() {
                sensorHandler.resetCalibrationData(String.valueOf(calibrationStep), calibrationPosition);
                switch (calibrationStep) {
                    case Gyro:
                        gyroCalibration();
                        break;
                    case Accel:
                        accelCalibration();
                        break;
                    case Mag:
                        magCalibration();
                        break;
                }
            }
        }.start();
    }

    private void playBeep(boolean isStart) {
        try {
            ToneGenerator toneGen = new ToneGenerator(AudioManager.STREAM_MUSIC, 100);
            if (isStart)
                toneGen.startTone(ToneGenerator.TONE_CDMA_CONFIRM, 500);
            else
                toneGen.startTone(ToneGenerator.TONE_PROP_BEEP, 500);
        } catch (RuntimeException e) {
            Log.e("BeepError", "ToneGenerator failed: " + e.getMessage());
        }
    }

    private void gyroCalibration() {
        playBeep(false);
        calibrationTimer = new CountDownTimer(TOTAL_CALIB_TIME, TIME_INTERVAL) {
            @Override
            public void onTick(long millisUntilFinished) {
                float secondsRemaining = (float) millisUntilFinished / 1000;
                textViewTimer.setText(String.format("Gyro Calibration: %.1fs", secondsRemaining));
                sensorHandler.sampleGyro();
            }

            @Override
            public void onFinish() {
                sensorHandler.calibrateGyro();
                doCalibration();
            }
        }.start();
    }

    private void accelCalibration() {
        playBeep(false);
        calibrationTimer = new CountDownTimer(TOTAL_CALIB_TIME, TIME_INTERVAL) {
            @Override
            public void onTick(long millisUntilFinished) {
                float secondsRemaining = (float) millisUntilFinished / 1000;
                textViewTimer.setText(String.format("Accel Position %d/6: %.1fs", calibrationPosition+1, secondsRemaining));
                sensorHandler.sampleAccel(calibrationPosition);
            }

            @Override
            public void onFinish() {
                sensorHandler.calibrateAccel();
                doCalibration();
            }
        }.start();
    }

    private void magCalibration() {
        playBeep(false);
        calibrationTimer = new CountDownTimer(TOTAL_CALIB_TIME, TIME_INTERVAL) {
            @Override
            public void onTick(long millisUntilFinished) {
                float secondsRemaining = (float) millisUntilFinished / 1000;
                textViewTimer.setText(String.format("Mag Calibration: %.1fs", secondsRemaining));
                sensorHandler.sampleMag();
            }

            @Override
            public void onFinish() {
                sensorHandler.calibrateMag();
                doCalibration();
            }
        }.start();
    }

    public void printSensorData(Map<String, float[]> data) {
        float[] accel = data.get("accel");
        float[] gyro = data.get("gyro");
        float[] mag = data.get("mag");
        textViewSensor.setText(String.format("Raw Gyro: %.2f, %.2f, %.2f\nRaw Accel: %.2f, %.2f, %.2f\nRaw Magnet: %.2f, %.2f, %.2f",
                gyro[0], gyro[1],gyro[2],
                accel[0],accel[1],accel[2],
                mag[0],mag[1],mag[2]));
    }
}