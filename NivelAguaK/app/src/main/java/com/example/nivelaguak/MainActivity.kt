package com.example.nivelaguak

import android.net.ConnectivityManager
import android.net.NetworkCapabilities
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.annotation.RequiresApi
import androidx.appcompat.app.AppCompatActivity
import com.google.gson.Gson
import okhttp3.*
import java.io.IOException

class MainActivity : AppCompatActivity() {

    private lateinit var tvNivelAgua: TextView
    private lateinit var btnEncenderBomba: Button
    private lateinit var btnApagarBomba: Button

    // URLs ajustadas con las claves API correctas
    private val apiUrl = "https://api.thingspeak.com/channels/2782709/feeds.json?api_key=Y110M1MLLT8Q74CR&results=1"
    private val encenderBombaUrl = "https://api.thingspeak.com/update?api_key=EW65PRKCOEEZ50GI&field1=1"
    private val apagarBombaUrl = "https://api.thingspeak.com/update?api_key=EW65PRKCOEEZ50GI&field1=0"
    private val client = OkHttpClient()

    private val handler = Handler(Looper.getMainLooper())
    private val updateInterval = 5000L // Actualización cada 5 segundos

    @RequiresApi(Build.VERSION_CODES.M)
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        // Inicializar vistas
        tvNivelAgua = findViewById(R.id.tvNivelAgua)
        btnEncenderBomba = findViewById(R.id.btnEncenderBomba)
        btnApagarBomba = findViewById(R.id.btnApagarBomba)

        // Iniciar actualización automática
        startAutoUpdate()

        // Controlar la bomba de agua
        btnEncenderBomba.setOnClickListener {
            if (isInternetAvailable()) {
                controlarBomba(encenderBombaUrl, "Bomba encendida!")
            } else {
                mostrarErrorConexion()
            }
        }

        btnApagarBomba.setOnClickListener {
            if (isInternetAvailable()) {
                controlarBomba(apagarBombaUrl, "Bomba apagada!")
            } else {
                mostrarErrorConexion()
            }
        }
    }

    private fun startAutoUpdate() {
        handler.post(object : Runnable {
            override fun run() {
                actualizarNivelAgua()
                handler.postDelayed(this, updateInterval)
            }
        })
    }

    private fun actualizarNivelAgua() {
        val request = Request.Builder().url(apiUrl).build()
        client.newCall(request).enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) {
                runOnUiThread {
                    tvNivelAgua.text = "Error al obtener datos"
                    Toast.makeText(this@MainActivity, "Error al conectar con ThingSpeak", Toast.LENGTH_SHORT).show()
                    Log.e("ActualizarNivelAgua", "Error: ${e.message}")
                }
            }

            override fun onResponse(call: Call, response: Response) {
                response.body?.string()?.let { jsonResponse ->
                    val nivelAgua = parseNivelAguaFromJson(jsonResponse)
                    runOnUiThread {
                        if (nivelAgua != null) {
                            tvNivelAgua.text = "Nivel de agua: $nivelAgua%"
                            if (nivelAgua == "0") {
                                Toast.makeText(this@MainActivity, "El estanque está vacío", Toast.LENGTH_SHORT).show()
                            }
                        } else {
                            tvNivelAgua.text = "Error al procesar datos"
                        }
                    }
                } ?: runOnUiThread {
                    tvNivelAgua.text = "Respuesta vacía del servidor"
                }
            }
        })
    }

    private fun controlarBomba(url: String, successMessage: String) {
        val request = Request.Builder().url(url).build()
        client.newCall(request).enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) {
                runOnUiThread {
                    Toast.makeText(this@MainActivity, "Error al controlar la bomba", Toast.LENGTH_SHORT).show()
                    Log.e("ControlarBomba", "Error: ${e.message}")
                }
            }

            override fun onResponse(call: Call, response: Response) {
                runOnUiThread {
                    if (response.isSuccessful) {
                        Toast.makeText(this@MainActivity, successMessage, Toast.LENGTH_SHORT).show()
                    } else {
                        Toast.makeText(this@MainActivity, "Error al enviar comando", Toast.LENGTH_SHORT).show()
                    }
                }
            }
        })
    }

    private fun parseNivelAguaFromJson(json: String): String? {
        return try {
            val gson = Gson()
            val response = gson.fromJson(json, ThingSpeakResponse::class.java)
            response.feeds.getOrNull(0)?.field1
        } catch (e: Exception) {
            Log.e("ParseNivelAgua", "Error al parsear JSON: ${e.message}")
            null
        }
    }

    @RequiresApi(Build.VERSION_CODES.M)
    private fun isInternetAvailable(): Boolean {
        val connectivityManager = getSystemService(CONNECTIVITY_SERVICE) as ConnectivityManager
        val network = connectivityManager.activeNetwork
        val capabilities = connectivityManager.getNetworkCapabilities(network)
        return capabilities != null && capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
    }

    override fun onDestroy() {
        super.onDestroy()
        handler.removeCallbacksAndMessages(null)
    }

    private fun mostrarErrorConexion() {
        Toast.makeText(this, "Sin conexión a Internet", Toast.LENGTH_SHORT).show()
    }
}

data class ThingSpeakResponse(val feeds: List<Feed>)
data class Feed(val field1: String?)