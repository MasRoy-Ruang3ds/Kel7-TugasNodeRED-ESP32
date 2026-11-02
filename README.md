# Tugas Praktikum MQTT - Sistem Monitoring Lingkungan dan Kendali Lampu Multi-Ruangan (Kelompok 7)

Proyek ini adalah tugas praktikum IoT untuk membuat prototipe sistem monitoring suhu dan kelembapan serta kontrol lampu 2 arah pada 4 ruangan (ruang1 sampai ruang4) secara real-time. Komunikasi antar perangkat menggunakan protokol MQTT dengan broker lokal Eclipse Mosquitto. Karena di lab tidak tersedia modul DHT11, pembacaan suhu dan kelembapan disimulasikan memakai potensiometer yang dibaca via ADC ESP32, lalu divisualisasikan dan dikontrol melalui dashboard Node-RED serta browser HP.

---

## Link Video
* **Video Presentasi:** [Tonton Video Presentasi](https://www.youtube.com/watch?v=6P_qFfroWEw) 
* **Video Demo Alat:** [Tonton Video Demo Sistem](https://www.youtube.com/watch?v=LS0XWdWO-4g) 

---

## Anggota Kelompok & Pembagian Tugas

| No | Nama | NRP | Ruangan | Pembagian Tugas |
|:---:|---|:---:|:---:|---|
| 1 | Fransciskus Pandapotan Manik | 2124600114 | ruang1 & ruang2 | Setup broker Mosquitto di laptop, buat flow & UI dashboard Node-RED, koding dan uji coba ruang1 & ruang2. |
| 2 | Muhammad Jeffry Senatriarno | 2124600109 | ruang3 | Rangkai hardware ruang3, buat laporan tugas, rapikan dokumentasi repo GitHub (README.md), ambil foto/video pengujian. |
| 3 | Ramaida Kurnia Putri | 2124600120 | ruang4 | Rangkai komponen ESP32 ruang4 (potensiometer & 2 LED), bantu penulisan kode program Arduino, uji coba fungsional ruang4, mengedit video presentasi dan demo alat. |

---

## Arsitektur Sistem

Topologi komunikasi data dibuat terpusat menggunakan broker Mosquitto lokal di laptop. Masing-masing ESP32 bertindak sebagai publisher data sensor/status dan subscriber kontrol lampu. Node-RED bertindak sebagai pengolah data sekaligus penyedia antarmuka dashboard.

```text
[ESP32 ruang1]   [ESP32 ruang2]   [ESP32 ruang3]   [ESP32 ruang4]
  (Pot + 2 LED)    (Pot + 2 LED)    (Pot + 2 LED)    (Pot + 2 LED)
       │                │                │                │
       └────────────────┼────────────────┼────────────────┘
                        │
                        ▼ (Publish: sensor & LWT status)
                        ▲ (Subscribe: kontrol LED)
              [ MQTT Broker Mosquitto ]
                IP: 172.20.10.2 : 1883
                        │
                        ▼ (Subscribe wildcard: pens/klp7/+/...)
                        ▲ (Publish switch lampu)
                 [ Node-RED Engine ]
                 Port Web UI: :1880
                        │
                        ▼ (HTTP Stream)
             [ Dashboard Web & Mobile ]
          http://172.20.10.2:1880/ui
```

---

## Perangkat Keras

### 1. Komponen yang Dipakai
* 4 unit ESP32 DevKit V1
* 4 unit Potensiometer 10k Ohm (pengganti sensor DHT11)
* 8 unit LED 5mm (masing-masing ruangan 2 LED)
* 8 unit Resistor 220 - 330 Ohm
* Breadboard dan kabel jumper secukupnya

### 2. Konfigurasi Pin ESP32 (Wiring)
Semua board ESP32 di ruang 1 sampai 4 menggunakan pinout yang sama persis:

| Komponen | Pin ESP32 | Keterangan |
|---|:---:|---|
| Potensiometer VCC | `3V3` | Kaki luar 1 |
| Potensiometer GND | `GND` | Kaki luar 2 |
| Potensiometer Data | `GPIO34` | Kaki tengah (ADC1 12-bit, nilai $0 - 4095$) |
| LED Lampu 1 | `GPIO25` | Kaki Anoda (+) lewat resistor 220 Ohm, Katoda (-) ke GND |
| LED Lampu 2 | `GPIO26` | Kaki Anoda (+) lewat resistor 220 Ohm, Katoda (-) ke GND |

### 3. Foto Rangkaian
Berikut foto hardware 4 ruangan yang sudah kami rangkai di breadboard:

![Foto Rangkaian Hardware](docs/images/rangkaian.jpg)

---

## Topik MQTT & Broker

* **Broker:** Mosquitto di Laptop (`172.20.10.2` port `1883`)
* **Prefix Kelompok:** `pens/klp7/`

| Topik MQTT | Arah | QoS | Retain | Contoh Payload | Keterangan |
|---|:---:|:---:|:---:|:---:|---|
| `pens/klp7/ruangN/temperature` | ESP32 -> Node-RED | 0 | false | `"27.8"` | Kirim data suhu tiap 5 detik |
| `pens/klp7/ruangN/humidity` | ESP32 -> Node-RED | 0 | false | `"55.4"` | Kirim data kelembapan tiap 5 detik |
| `pens/klp7/ruangN/status` | ESP32/Broker -> Node-RED | 1 | true | `"online"` / `"offline"` | Status koneksi board (memakai fitur LWT) |
| `pens/klp7/ruangN/lamp1` | Node-RED -> ESP32 | 1 | false | `"1"` atau `"0"` | Perintah saklar Lampu 1 |
| `pens/klp7/ruangN/lamp2` | Node-RED -> ESP32 | 1 | false | `"1"` atau `"0"` | Perintah saklar Lampu 2 |

> Catatan: Bagian `ruangN` disesuaikan dengan ruangan masing-masing (`ruang1`, `ruang2`, `ruang3`, `ruang4`). Di Node-RED kita gunakan wildcard `+` pada topik suhu agar 1 chart bisa langsung menangkap data 4 ruangan.

---

## Langkah Menjalankan Sistem

### 1. Upload Kode ke ESP32
* Buka Arduino IDE, pastikan sudah install library **EspMQTTClient** by Patrick Lapointe lewat Library Manager.
* Buka file `ESP32_ruang.ino`.
* Ubah baris `#define ROOM` sesuai board yang mau di-flash, serta samakan SSID dan password Wi-Fi:
  ```cpp
  // Cukup ubah bagian ini saat mau upload ke ESP32 lain
  #define KELOMPOK "klp7"
  #define ROOM     "ruang1" // ganti: ruang1 / ruang2 / ruang3 / ruang4

  const char* WIFI_SSID     = "SSID_LAB";
  const char* WIFI_PASSWORD = "PASSWORD_LAB";
  const char* MQTT_SERVER   = "172.20.10.2";
  const uint16_t MQTT_PORT  = 1883;
  ```
* Sambungkan kabel USB, pilih board **DOIT ESP32 DEVKIT V1**, pilih port COM yang sesuai, lalu klik **Upload**.
* Ulangi langkah di atas untuk board ruang2, ruang3, dan ruang4.

### 2. Setup Broker Mosquitto
* Buka file konfigurasi `mosquitto.conf` di laptop broker, tambahkan baris ini di paling bawah agar bisa diakses dari luar localhost:
  ```text
  listener 1883 0.0.0.0
  allow_anonymous true
  ```
* Buka Windows Services (`services.msc`), cari Mosquitto Broker lalu klik **Restart**.
* Pastikan port 1883 sudah diizinkan lewat Windows Defender Firewall.

### 3. Setup Node-RED Dashboard
* Buka Node-RED di browser: `http://localhost:1880`.
* Install palette dashboard: klik menu pojok kanan atas -> **Manage palette** -> tab **Install** -> cari `node-red-dashboard` -> klik **Install**.
* Import flow: klik Menu -> **Import** -> pilih file `flows.json` dari repositori ini -> klik **Import**.
* Klik tombol merah **Deploy** di kanan atas.
* Buka tampilan dashboard di browser:
  * Dari laptop broker: `http://localhost:1880/ui`
  * Dari HP atau laptop lain (asal 1 Wi-Fi): `http://172.20.10.2:1880/ui`

---

## Screenshot Node-RED & Dashboard

### 1. Flow Node-RED
Flow terdiri dari blok penerima suhu (wildcard chart), blok gauge kelembapan per ruang, penampil status teks, dan switch kontrol LED:

![Flow Node-RED](docs/images/flow.jpg)

### 2. Dashboard UI
Tampilan dashboard memuat 1 grafik garis gabungan 4 ruang, 4 gauge kelembapan, status online/offline, dan switch toggle lampu:

![Dashboard Node-RED](docs/images/dashboard.png)

---

## Hasil Pengujian (T1 - T12)

Pengujian sistem dilakukan sesuai dengan 12 skenario pengujian tugas:

| Kode | Item Pengujian | Prosedur Uji | Status | Hasil & Keterangan |
|:---:|---|---|:---:|---|
| T1 | Koneksi Broker | Buka Serial Monitor ESP32 ruang1 | ✓ | Terhubung ke IP `172.20.10.2:1883`, pesan ONLINE terkirim. |
| T2 | Data 4 Ruangan | Pantau node debug di Node-RED | ✓ | Data suhu & kelembapan masuk bergantian tiap 5 detik dari 4 board. |
| T3 | Chart Suhu Gabungan | Buka dashboard web di browser | ✓ | Grafik garis menampilkan data 4 ruangan sekaligus dengan warna berbeda. |
| T4 | Perubahan Input Sensor | Putar kenop potensiometer di ruang tertentu | ✓ | Hanya nilai suhu dan gauge ruangan yang diputar yang berubah nilainya. |
| T5 | Lampu Tepat Sasaran | Klik saklar Lampu 2 ruang2 / ruang3 di dashboard | ✓ | Payload 1 sukses menyalakan LED di ruangan tujuan tanpa menyasar ruang lain. |
| T6 | Mematikan Lampu | Matikan switch saklar lampu di dashboard | ✓ | Payload 0 mematikan LED tujuan dengan normal. |
| T7 | Filter Payload Salah | Kirim payload sembarang (misal `"ON"`) lewat MQTT | ✓ | Payload salah diabaikan oleh logika if-else ESP32, sistem tidak error. |
| T8 | Indikator Status Online | Cek status di dashboard saat board hidup | ✓ | Indikator teks hijau bertuliskan ONLINE muncul real-time. |
| T9 | Uji Last Will (LWT) | Cabut kabel power ESP32 tiba-tiba | ✓ | Setelah $\pm 22$ detik broker mendeteksi koneksi putus dan status otomatis berubah OFFLINE. |
| T10 | Uji Retained Message | Lakukan deploy ulang / restart Node-RED | ✓ | Status terakhir langsung muncul tanpa harus nunggu ESP32 publish ulang (karena retain=true). |
| T11 | Auto-Reconnect | Matikan Wi-Fi laptop selama 10 detik lalu nyalakan lagi | ✓ | ESP32 otomatis tersambung kembali ke Wi-Fi dan MQTT tanpa harus di-reset manual. |
| T12 | Akses via HP | Buka URL dashboard lewat browser HP | ✓ | Tampilan dashboard responsif dan saklar lampu bisa diklik normal dari HP. |

---

## Ruangan yang Disimulasikan

Pada tugas ini kelompok kami **menggunakan 4 unit modul ESP32 fisik secara lengkap**, jadi tidak ada ruangan yang disimulasikan secara software / dummy:
* **ESP32 1:** Hardware fisik untuk `ruang1`
* **ESP32 2:** Hardware fisik untuk `ruang2`
* **ESP32 3:** Hardware fisik untuk `ruang3`
* **ESP32 4:** Hardware fisik untuk `ruang4`

---

## Kendala dan Solusi

1. **Komponen Sensor DHT11 Tidak Ada**
   * *Kendala:* Modul sensor suhu/kelembapan DHT11 tidak tersedia di lab saat perakitan.
   * *Solusi:* Menggunakan potensiometer sebagai simulator sensor analog. Nilai ADC ($0 - 4095$) kami konversi di kodingan ESP32:
     * Suhu ($15 - 40^\circ\text{C}$): `15.0f + (raw / 4095.0f) * 25.0f`
     * Kelembapan ($20 - 90\%$): `20.0f + (raw / 4095.0f) * 70.0f`

2. **ESP32 Gagal Konek ke Mosquitto (Muncul Pesan "MQTT belum terhubung")**
   * *Kendala:* Mosquitto versi baru di Windows default-nya hanya membolehkan koneksi dari localhost, dan port 1883 terblokir Windows Firewall.
   * *Solusi:* Edit file `mosquitto.conf`, tambahkan konfigurasi `listener 1883 0.0.0.0` dan `allow_anonymous true`, restart service Mosquitto, lalu izinkan port 1883 pada inbound rule Windows Firewall.
