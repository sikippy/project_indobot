# project_indobot<br/>
proyek penyiraman anggrek<br/>

link project akhir:<br/>
https://wokwi.com/projects/346563201721172563<br/><br/>

Kontribusi :
Ayu Roshida     : https://wokwi.com/projects/341152728173511250 <br/>
Diana Catur     : https://wokwi.com/projects/346550144303366738 <br/>
Dwi Mayasari    : prototipe <br/>
Herni Bunga     : https://thingsboard.cloud/deviceGroups/e38b8c10-54c4-11ed-8ab1-ebe8b1422014 <br/>
Laras Istiqomah : Slide dan Presentasi https://docs.google.com/presentation/d/1nJLA6wzpvZaa4lZ03OP1LcB2QiDQK0eB/edit?usp=sharing&ouid=116947196870439495696&rtpof=true&sd=true <br/>
Vina P.         : https://wokwi.com/projects/346501431045390930

Pengetahuan tentang anggrek <br/>

1. pilih media tanam yang cocok <br/>
2.  tidak di tempatkan di sinar matahari langsung <br/>
3. warna daun memberikan informasi jika menggelap butuh lebih banyak cahaya.<br/>
4. penyiraman di suhu ruangan. <br/>
5. siram selama 15 detik <br/>
6. jangan siram lebih 1 x dari seminggu<br/> 
7. butuh suhu 10-30 derajat celcius <br/>
8. pemupukan 2 kali seminggu <br/>
9. air tidak boleh terlalu panas atau dingin, perlu diukur juga suhu airnya <br/>


https://www.sehatq.com/review/cara-merawat-anggrek-agar-tumbuh-subur-dan-berbunga <br/>
https://www.kompas.com/homey/read/2021/10/14/205500676/12-kesalahan-dalam-merawat-anggrek-yang-harus-dihindari?page=all <br/>

Teknikal proses: <br/>

Sensor input yang digunakan : <br/>
1. LDR untuk membantu mengukur cahaya yang diperlukan anggrek, perlu melihat warna daun, jika ada camera dan bisa mengirimkan foto, maka akan mampu     mengirimkan notifikasi bahwa tanaman anggrek butuh lebih banyak cahaya. <br/>
2. DHT22 untuk mengukur temperatur dan suhu. <br/>
3. PIR untuk mendeteksi pergerakan objek, sehingga dapat menghentikan penyiraman. <br/>
4. Push button untuk manual jika terjadi kesalahan pada pengoperasian otomatis. <br/>

Sensor output yang digunakan : <br/>
1. RTC untuk menghitung waktu real time saat eksekusi proses.<br/>
2. Relay untuk mengendalikan arus listrik yang mengalir, bisa digunakan juga untuk menyalakan dan mematikan lampu.<br/>
3. Buzzer untuk menghasilkan bunyi berdasarkan proses.<br/>
4. LED untuk memberikan informasi proses, warning akan menghidupkan LED <br/>


