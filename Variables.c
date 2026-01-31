/* ---------- Deðiþken tanýmlamalarý ---------- */ 

// Temel Deðiþken Tanýmlamalarý
int counter;                 // Varsayýlan olarak RAM’de yer alan, iþareti olan tam sayý deðiþken
unsigned int tick;           // Negatif deðer almayan sayaçlar için tercih edilir
char rxByte;                 // UART vb. haberleþmelerde tek bayt veri için kullanýlýr
float temperature;           // Sensörlerden gelen ondalýklý veriler için

// STM32’de Sýk Kullanýlan Sabit Geniþlikli Tipler (stdint.h). Gömülü sistemlerde taþýnabilirlik ve bellek kontrolü için bu tipler kritik önemdedir.
uint8_t  uartData;            // 8-bit, UART RX/TX için ideal
uint16_t adcValue;            // 12-bit ADC verileri genelde 16-bit deðiþkende tutulur
uint32_t systemTick;          // SysTick veya zamanlayýcý sayaçlarý için
int16_t  motorSpeed;          // Pozitif/negatif yönlü motor hýz bilgisi

// Baþlangýç Deðeri Verilerek Tanýmlama. STM32 tarafýnda ilk deðer verilmemiþ deðiþkenler RAM’de rastgele deðer içerebilir.
uint8_t ledState = 0;         // LED baþlangýçta kapalý
uint16_t adcRaw = 0;          // ADC ölçümü baþlamadan önce sýfýrlanýr
float voltage = 0.0f;         // Hesaplamalarda belirsizlik olmamasý için

//const Kullanýmý (Flash Bellek Odaklý). Sabitler Flash bellekte tutulur, RAM tüketmez.
const uint16_t ADC_MAX = 4095;    // 12-bit ADC maksimum deðeri
const float VREF = 3.3f;          // Referans voltaj sabiti

// volatile Kullanýmý (Donaným Register / ISR). Donaným tarafýndan deðiþebilen deðiþkenler için zorunludur.
volatile uint8_t uartRxFlag;       // UART interrupt içinde deðiþtirilen bayrak
volatile uint32_t msCounter;       // Timer interrupt ile artan sayaç

// Global ve Local Deðiþken Örneði
uint8_t errorCode;                 // Global deðiþken, tüm dosyada eriþilebilir
void ReadSensor(void)
{
    uint16_t sensorValue;          // Local deðiþken, sadece fonksiyon içinde geçerli
    sensorValue = 1234;            // Sensör okuma simülasyonu
}

// STM32 HAL Tarzý Tip Tanýmlama (typedef). Kod okunabilirliðini ciddi þekilde artýrýr.
typedef uint8_t bool_t;            // Gömülü projelerde sýk kullanýlan boolean tanýmý
bool_t isSystemReady;              // Sistem hazýr mý bayraðý
