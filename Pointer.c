/* ---------- Pointer ---------- */

// Pointer, bir deðiþkenin RAM’deki adresini tutan deðiþkendir.
int value = 10;              // RAM’de bir tamsayý deðiþken
int *pValue;                 // int tipinde bir deðiþkenin adresini tutacak pointer
pValue = &value;             // value deðiþkeninin adresi pValue içine atanýr

// Pointer ile Deðere Eriþim (Dereference). * operatörü ile pointer’ýn gösterdiði adresteki deðere ulaþýlýr. STM32 tarafýnda bu mekanizma register eriþiminin temelidir.
int value = 10;              // Normal deðiþken
int *pValue = &value;        // value’nun adresi pointer’a atanýr
value = 20;                  // Deðiþken doðrudan deðiþtirilir
*pValue = 30;                // Pointer üzerinden ayný deðiþken deðiþtirilir

// Pointer ve Sabit Geniþlikli Tipler (stdint.h). Gömülü sistemlerde pointer tipi, iþaret edilen veriyle birebir uyumlu olmalýdýr.
uint16_t adcValue = 0;       // ADC ölçüm deðeri
uint16_t *pAdcValue;         // 16-bit veri gösteren pointer
pAdcValue = &adcValue;       // Adres atamasý

// Fonksiyonlara Pointer ile Parametre Gönderme. STM32 HAL ve driver yapýlarýnda çok sýk kullanýlýr.
void ReadADC(uint16_t *value)    // ADC sonucunu yazmak için pointer alýnýr
{
    *value = 2048;               // Ölçüm sonucu pointer üzerinden yazýlýr
}

int main(void)
{
    uint16_t adcResult = 0;      // ADC sonucu tutulacak deðiþken
    ReadADC(&adcResult);         // Deðiþkenin adresi fonksiyona gönderilir
}

// Pointer ile Donaným Register Eriþimi (Temel Mantýk). STM32 register’larý belirli bellek adreslerindedir. Bu yapý: CMSIS HAL Bare-metal STM32 kodlarýnýn temelidir
#define GPIOA_ODR   ((uint32_t*)0x48000014)   // GPIOA Output Data Register adresi
*GPIOA_ODR = 0x00000001;       // PA0 pinini 1 yapar (LED yakma örneði)

// volatile ve Pointer Birlikteliði. Donaným register’larý volatile olmak zorundadýr.
#define GPIOA_IDR   ((volatile uint32_t*)0x48000010)   // GPIOA Input Data Register
uint32_t buttonState;
buttonState = *GPIOA_IDR;      // Buton durumu okunur

// Pointer ile Dizi Ýliþkisi (STM32 Buffer Kullanýmý). UART, SPI, I2C buffer mantýðý bu yapý üzerine kuruludur.
uint8_t rxBuffer[10];          // UART RX buffer
uint8_t *pRxBuffer;            // Buffer baþlangýcýný gösteren pointer
pRxBuffer = rxBuffer;          // Dizinin ilk eleman adresi

