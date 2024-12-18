package com.example.nivelagua;

import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.loopj.android.http.AsyncHttpClient;
import com.loopj.android.http.AsyncHttpResponseHandler;
import com.loopj.android.http.RequestParams;
import org.json.JSONObject;
import cz.msebera.android.httpclient.Header;

public class MainActivity extends AppCompatActivity {

    private TextView tvNivelAgua;
    private Button btnEncenderBomba, btnApagarBomba;

    private final String READ_API_KEY = "BIQ169NLKIRSRNIO";
    private final String WRITE_API_KEY = "A0T3NQX2W7RSGMOW";
    private final String CHANNEL_ID = "2782709";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        tvNivelAgua = findViewById(R.id.tvNivelAgua);
        btnEncenderBomba = findViewById(R.id.btnEncenderBomba);
        btnApagarBomba = findViewById(R.id.btnApagarBomba);

        // Actualizar nivel de agua periódicamente
        actualizarNivelAgua();

        // Encender bomba
        btnEncenderBomba.setOnClickListener(v -> controlarBomba(1));

        // Apagar bomba
        btnApagarBomba.setOnClickListener(v -> controlarBomba(0));
    }

    private void actualizarNivelAgua() {
        String url = "https://api.thingspeak.com/update?api_key=A0T3NQX2W7RSGMOW&field1=0";

        AsyncHttpClient client = new AsyncHttpClient();
        client.get(url, new AsyncHttpResponseHandler() {
            @Override
            public void onSuccess(int statusCode, Header[] headers, byte[] responseBody) {
                try {
                    String response = new String(responseBody);
                    JSONObject jsonObject = new JSONObject(response);
                    JSONObject feed = jsonObject.getJSONArray("feeds").getJSONObject(0);
                    String nivelAgua = feed.getString("field1");

                    if (nivelAgua != null && !nivelAgua.isEmpty()) {
                        int nivel = Integer.parseInt(nivelAgua);
                        tvNivelAgua.setText("Nivel de agua: " + nivel + "%");

                        if (nivel == 0) {
                            Toast.makeText(MainActivity.this, "El estanque está vacío", Toast.LENGTH_SHORT).show();
                        }
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    Toast.makeText(MainActivity.this, "Error al leer el nivel de agua", Toast.LENGTH_SHORT).show();
                }
            }

            @Override
            public void onFailure(int statusCode, Header[] headers, byte[] responseBody, Throwable error) {
                Toast.makeText(MainActivity.this, "Error al conectar con ThingSpeak", Toast.LENGTH_SHORT).show();
            }
        });
    }

    private void controlarBomba(int estado) {
        String url = "https://api.thingspeak.com/channels/2782709/feeds.json?api_key=BIQ169NLKIRSRNIO&results=2";
        AsyncHttpClient client = new AsyncHttpClient();

        RequestParams params = new RequestParams();
        params.put("api_key", WRITE_API_KEY);
        params.put("field2", estado);

        client.post(url, params, new AsyncHttpResponseHandler() {
            @Override
            public void onSuccess(int statusCode, Header[] headers, byte[] responseBody) {
                String estadoBomba = estado == 1 ? "encendida" : "apagada";
                Toast.makeText(MainActivity.this, "Bomba " + estadoBomba, Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onFailure(int statusCode, Header[] headers, byte[] responseBody, Throwable error) {
                Toast.makeText(MainActivity.this, "Error al controlar la bomba", Toast.LENGTH_SHORT).show();
            }
        });
    }
}