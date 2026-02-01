/* ---------- Struct ---------- */ 

// struct, farklý tipteki verileri tek bir mantýksal yapý altýnda toplamak için kullanýlýr. Gömülü sistemlerde bu genellikle peripheral, sensör veya konfigürasyon modeli anlamýna gelir.
struct SensorData
{
    uint16_t raw;             // ADC’den okunan ham deðer
    float voltage;            // Hesaplanan voltaj
    uint8_t status;           // Sensör durumu
};

// struct Deðiþkeni Tanýmlama ve Kullanma
struct SensorData sensor1;    // SensorData tipinde deðiþken

sensor1.raw = 2000;           // Struct üyesine eriþim
sensor1.voltage = 1.65f;      // Nokta (.) operatörü kullanýlýr
sensor1.status = 1;           // Sensör aktif

// typedef ile Daha Okunabilir Struct Tanýmý. STM32 projelerinde standart kullaným þeklidir.
typedef struct
{
    uint16_t raw;             // ADC ham veri
    float voltage;            // Voltaj deðeri
    uint8_t status;           // Sensör durumu
} Sensor_t;

Sensor_t sensor1;             // Artýk struct keyword gerekmez
sensor1.raw = 3000;           // Üye eriþimi
sensor1.voltage = 2.4f;       // Hesaplanan deðer
sensor1.status = 0;           // Sensör pasif

// Struct Pointer Kullanýmý (-> Operatörü). Driver ve HAL fonksiyonlarýnda en sýk kullanýlan yapý.
Sensor_t sensor1;             // Sensor yapýsý
Sensor_t *pSensor;            // Sensor yapýsýný gösteren pointer

pSensor = &sensor1;            // Struct adresi pointer’a atanýr

pSensor->raw = 1500;           // Pointer ile struct üyesine eriþim
pSensor->voltage = 1.2f;       // -> operatörü kullanýlýr
pSensor->status = 1;           // Sensör aktif

// Fonksiyonlara Struct Pointer Gönderme. STM32 driver mimarisinin temelidir.
void Sensor_Update(Sensor_t *sensor)   // Struct pointer parametre olarak alýnýr
{
    sensor->raw = 2048;                // ADC okuma simülasyonu
    sensor->voltage = 1.65f;           // Voltaj hesaplanýr
    sensor->status = 1;                // Sensör geçerli
}

int main(void)
{
    Sensor_t mySensor;                 // Sensor instance
    Sensor_Update(&mySensor);           // Adres fonksiyona gönderilir
}

// Struct ile Peripheral Konfigürasyonu (STM32 Tarzý). HAL yapýlarýna birebir benzer örnek.
typedef struct
{
    uint32_t baudRate;         // UART baud rate
    uint8_t dataBits;          // Veri bit sayýsý
    uint8_t stopBits;          // Stop bit sayýsý
} UART_Config_t;

UART_Config_t uart1Config;     // UART konfigürasyon yapýsý

uart1Config.baudRate = 115200; // UART hýzý
uart1Config.dataBits = 8;      // 8-bit veri
uart1Config.stopBits = 1;      // 1 stop bit

// Struct Dizisi (Birden Fazla Sensör / Peripheral)
Sensor_t sensors[3];           // 3 adet sensör yapýsý

sensors[0].raw = 1000;         // Ýlk sensör
sensors[1].raw = 2000;         // Ýkinci sensör
sensors[2].raw = 3000;         // Üçüncü sensör

// Embedded Perspektifinden Neden Struct?
//STM32 tarafýnda struct kullanýmý: Peripheral konfigürasyonlarý, Driver state yönetimi, Sensör soyutlama, Kod okunabilirliði, Bakým ve geniþletilebilirlik için kritik önemdedir.

// Pointer + Struct = STM32 HAL Mantýðý (Kavramsal). Bu yapý, STM32’nin CMSIS register haritalamasýnýn temel fikridir.
typedef struct
{
    volatile uint32_t *ODR;    // GPIO Output Data Register adresi
    volatile uint32_t *IDR;    // GPIO Input Data Register adresi
} GPIO_Port_t;
