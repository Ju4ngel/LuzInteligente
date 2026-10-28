package com.juanangel.luzinteligente

import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import com.google.firebase.auth.FirebaseAuth
import com.google.firebase.database.FirebaseDatabase
import com.google.firebase.database.ServerValue
import kotlin.math.abs

class MainActivity : AppCompatActivity(), SensorEventListener {

    private val dbUrl = "https://luz-inteligente-2f973-default-rtdb.firebaseio.com"

    private lateinit var sensorManager: SensorManager
    private var lightSensor: Sensor? = null
    private lateinit var tvLux: TextView
    private lateinit var tvEstado: TextView

    private val auth = FirebaseAuth.getInstance()
    private val ref by lazy {
        FirebaseDatabase.getInstance(dbUrl).getReference("iluminacion")
    }

    private var autenticado = false
    private var ultimoEnvio = 0L
    private var ultimoLux = -1f

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        tvLux = findViewById(R.id.tvLux)
        tvEstado = findViewById(R.id.tvEstado)

        sensorManager = getSystemService(SENSOR_SERVICE) as SensorManager
        lightSensor = sensorManager.getDefaultSensor(Sensor.TYPE_LIGHT)

        if (lightSensor == null) {
            tvEstado.text = "Este dispositivo no tiene sensor de luz"
        }

        iniciarSesion()
    }

    private fun iniciarSesion() {
        if (auth.currentUser != null) {
            autenticado = true
            tvEstado.text = "Conectado a Firebase"
            return
        }
        auth.signInAnonymously()
            .addOnSuccessListener {
                autenticado = true
                tvEstado.text = "Conectado a Firebase"
            }
            .addOnFailureListener { e ->
                tvEstado.text = "Error de login: ${e.message}"
            }
    }

    override fun onResume() {
        super.onResume()
        lightSensor?.let {
            sensorManager.registerListener(this, it, SensorManager.SENSOR_DELAY_NORMAL)
        }
    }

    override fun onPause() {
        super.onPause()
        sensorManager.unregisterListener(this)
    }

    override fun onSensorChanged(event: SensorEvent) {
        val lux = event.values[0]
        tvLux.text = "${lux.toInt()} lux"

        val ahora = System.currentTimeMillis()
        val pasoTiempo = ahora - ultimoEnvio >= 500
        val cambioSignificativo = abs(lux - ultimoLux) >= 1f

        if (autenticado && pasoTiempo && cambioSignificativo) {
            ultimoEnvio = ahora
            ultimoLux = lux
            val datos = mapOf(
                "lux" to lux.toInt(),
                "timestamp" to ServerValue.TIMESTAMP
            )
            ref.updateChildren(datos)
                .addOnFailureListener { e ->
                    tvEstado.text = "Error al enviar: ${e.message}"
                }
        }
    }

    override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {}
}