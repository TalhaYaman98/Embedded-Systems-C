# Gömülü C Pratikleri

STM32 hedefli, MISRA-C:2012 uyumlu C dili çalışma notları.

---

## İçerik

| Dosya | Konu |
|---|---|
| `Veriables.c` | Temel tipler, stdint, initialization, scope, casting, sabit tanımlama, struct |
| `Functions.c` | Prototip, dönüş tipleri, parametre geçişi, inline, callback, özyineleme |
| `Pointer.c` | Temel pointer, aritmetik, dizi ilişkisi, pointer to pointer, void pointer, const kombinasyonları |
| `Bitwise.c` | Temel operatörler, bit set/clear/toggle, maskeleme, aritmetik, union ile register erişimi |
| `Control_Structures.c` | if/else, switch, for, while, do-while, iç içe döngü |
| `Memory_Management.c` | Stack, static, global, statik allocasyon, ring buffer, bellek hizalama |
| `Preprocessor.c` | #define, fonksiyon benzeri makro, koşullu derleme, header guard, platform soyutlama |
| `Data_Structures.c` | Linked list, queue, ring buffer |
| `Interrupt.c` | ISR flag yönetimi, ring buffer, critical section, zamanlayıcı |
| `State_Machine.c` | Switch/case, fonksiyon pointer, tablo tabanlı durum makinesi |
| `Watchdog.c` | IWDG, WWDG, görev izleme, reset nedeni tespiti |
| `Power_Management.c` | Uyku modları, wakeup kaynakları, saat yönetimi, güç tüketim hesabı |
| `Fault_Management.c` | Hata kodları, assert, fault handler, hata loglama |
| `Filters_For_Measurements.c` | Moving Average, WMA, EMA, Median, IIR, FIR, Kalman, Butterworth, Alpha-Beta, Debounce |

---

## Filtre Seçim Rehberi

| Ölçüm | Önerilen Filtre | Neden |
|---|---|---|
| ADC / gerilim | EMA | Basit, tek parametre, düşük bellek |
| Sıcaklık | EMA (α=0.05..0.1) | Yavaş sistem, yumuşak yanıt yeterli |
| Basınç | IIR / Butterworth | Frekans seçici filtreleme |
| Anlık spike | Median | Spike'ı tamamen yok eder |
| IMU / ivme | Kalman | Gürültü + model birlikte işlenir |
| Enkoder hız | Alpha-Beta | Konum + hız birlikte tahmin |
| Genel amaç | Moving Average | Anlaşılır, öngörülebilir davranış |
| Ses / titreşim | FIR | Doğrusal faz, kararlı |
| Buton | Debounce | Dijital titreşim temizleme |
| Motor akım | WMA | Yeni veriye daha duyarlı |

---

## MISRA-C:2012 Uygulamaları

Her dosyada uygulanan kurallar dosya başında listelenmiştir. Genel olarak uygulanan kurallar:

| Kural | Açıklama |
|---|---|
| Rule 7.2 | Unsigned sabit literaller `U` suffix taşımalı |
| Rule 8.1 | Tipler açıkça belirtilmeli |
| Rule 8.7 | Tek dosyadan erişilen nesne `static` olmalı |
| Rule 10.3 | Atama hedef tip ile uyumlu olmalı |
| Rule 10.4 | Aritmetik operandlar aynı esansiyel tipte olmalı |
| Rule 12.2 | Shift miktarı tip genişliğinden küçük olmalı |
| Rule 14.4 | `if`/`while` koşulu esansiyel boolean tipinde olmalı |
| Rule 15.2 | `switch` default etiketi olmalı |
| Rule 15.4 | `switch` case fall-through olmamalı |
| Rule 15.5 | Fonksiyonun tek `return` noktası olmalı |
| Rule 17.7 | Non-void fonksiyon dönüş değeri kullanılmalı |
| Rule 21.3 | `malloc`/`free` kullanımı yasak |

---

## Geliştirme Ortamı

| Bileşen | Versiyon |
|---|---|
| Derleyici | GCC 15.2 (MinGW-w64 UCRT64) |
| IDE | Visual Studio Code |
| Hedef platform | STM32 (Cortex-M) |
| C standardı | C99 |

---

## Platform Notları

Tüm dosyalar PC ortamında derlenip çalıştırılabilir. STM32'ye özgü satırlar
yorum içinde gösterilmiştir:

```c
/* STM32 HAL kodu:                          */
/* HAL_UART_Transmit(&huart1, buf, 4, 100); */

/* PC simulasyonu: */
printf("UART gonder: %u byte\n", (uint32_t)boy);
```

Gerçek STM32 projesine taşırken:
- `typedef float float32_t` satırını kaldır — `arm_math.h` içerir
- Yorum içindeki HAL fonksiyonlarını aktif et
- `printf` çıktılarını `UART_Transmit` ile değiştir
- `volatile` değişkenler için `__disable_irq()` / `__enable_irq()` ekle

---

## Klasör Yapısı

```
    ├── Veriables.c
    ├── Functions.c
    ├── Pointer.c
    ├── Bitwise.c
    ├── Control_Structures.c
    ├── Memory_Management.c
    ├── Preprocessor.c
    ├── Data_Structures.c
    ├── Interrupt.c
    ├── State_Machine.c
    ├── Watchdog.c
    ├── Power_Management.c
    ├── Fault_Management.c
    ├── Filters_For_Measurements.c
    └── README.md
```

---

## Kaynaklar

- [MISRA-C:2012 Guidelines](https://www.misra.org.uk)
- [STM32 HAL Kullanıcı Kılavuzu](https://www.st.com/resource/en/user_manual/um1786-stm32cube-hal-and-ll-drivers-for-stm32f4-series-stmicroelectronics.pdf)
- [ARM Cortex-M Programlama Kılavuzu](https://developer.arm.com/documentation)
- [Barr Group — Embedded C Kodlama Standardı](https://barrgroup.com/embedded-systems/books/embedded-c-coding-standard)
