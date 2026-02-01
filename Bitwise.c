/* ---------- Bitwise ---------- */ 

// Bitwise Operatörler
uint8_t a = 0x0A;             // 0000 1010
uint8_t b = 0x03;             // 0000 0011

uint8_t andVal = a & b;       // AND  › 0000 0010
uint8_t orVal  = a | b;       // OR   › 0000 1011
uint8_t xorVal = a ^ b;       // XOR  › 0000 1001
uint8_t notVal = ~a;          // NOT  › 1111 0101


// Bit Kaydýrma (<<, >>)
uint8_t value = 1;            // 0000 0001

value = value << 3;           // 0000 1000  › bit 3 set edilir
value = value >> 1;           // 0000 0100  › bit 2’ye kayar


// Tek Bir Bit Set Etme
uint32_t reg = 0;             // Register simülasyonu

reg |= (1U << 5);             // Bit 5 set edilir


// Tek Bir Bit Sýfýrlama (Clear)
uint32_t reg = 0xFFFFFFFF;    // Tüm bitler 1

reg &= ~(1U << 5);            // Bit 5 temizlenir


// Bit Toggle Etme. LED yak/söndür gibi iþlemler için.
uint32_t reg = 0;

reg ^= (1U << 5);             // Bit 5 toggle edilir


// Bir Bitin Durumunu Okuma. Buton, status flag kontrolü.
uint32_t reg = 0x20;          // Bit 5 = 1

if (reg & (1U << 5))          // Bit 5 set mi kontrolü
{
    /* bit 1 */
}


// Bit Maskesi ile Çoklu Bit Ayarý. Peripheral konfigürasyonlarýnda yaygýndýr.
#define MODE_MASK   (0x3U << 4)    // Bit 5:4 maskesi

uint32_t reg = 0;

reg &= ~MODE_MASK;                 // Önce ilgili bitler temizlenir
reg |=  (0x2U << 4);               // Mode = 2 ayarlanýr


// STM32 GPIO Register Mantýðý (Basit)
#define GPIO_ODR   (*(volatile uint32_t*)0x48000014)   // GPIO ODR adresi

GPIO_ODR |=  (1U << 0);        // PA0 set (LED ON)
GPIO_ODR &= ~(1U << 0);        // PA0 clear (LED OFF)


// Bit Alanlarý (Bit-field) — Dikkatli Kullaným. Donaným register’larý için her zaman önerilmez. Compiler baðýmlýlýðý vardýr.
typedef struct
{
    uint32_t EN   : 1;         // Enable biti
    uint32_t MODE : 2;         // Mode alaný
    uint32_t RES  : 29;        // Kalan bitler
} CTRL_Reg_t;


// C dilinde, (typedef) mevcut bir türe takma ad veya yeni bir isim vermek için kullanýlýr.
// C dilinde, birleþim (union) farklý veri türlerinin ayný bellek konumunda saklanmasýna olanak tanýyan, kullanýcý tanýmlý bir veri türüdür.
// union sayesinde ayný belleðe: 16-bit (data) 2×8-bit (bytes) 16×1-bit (bits) olarak eriþiyorsunuz.
typedef union {	
    uint16_t data;                 // 16-bit ham veri, register veya sensör verisi olarak kullanýlýr

    struct {
        uint8_t lsb;               // Least Significant Byte (düþük anlamlý bayt)
        uint8_t msb;               // Most Significant Byte (yüksek anlamlý bayt)
    } bytes;                       // Veriye byte bazýnda eriþim saðlar (UART, SPI için kullanýþlý)

    struct {
        uint8_t bit0  : 1;         // Bit 0
        uint8_t bit1  : 1;         // Bit 1
        uint8_t bit2  : 1;         // Bit 2
        uint8_t bit3  : 1;         // Bit 3
        uint8_t bit4  : 1;         // Bit 4
        uint8_t bit5  : 1;         // Bit 5
        uint8_t bit6  : 1;         // Bit 6
        uint8_t bit7  : 1;         // Bit 7
        uint8_t bit8  : 1;         // Bit 8
        uint8_t bit9  : 1;         // Bit 9
        uint8_t bit10 : 1;         // Bit 10
        uint8_t bit11 : 1;         // Bit 11
        uint8_t bit12 : 1;         // Bit 12
        uint8_t bit13 : 1;         // Bit 13
        uint8_t bit14 : 1;         // Bit 14
        uint8_t bit15 : 1;         // Bit 15
    } bits;                        // Veriye bit bazýnda eriþim saðlar (flag / status register)
} union_16bit_t;                   // 16-bit veri için çoklu eriþim modeli saðlayan union

/*

| Kriter           | Maske        | Bit-field |
| ---------------- | ------------ | --------- |
| Güvenilirlik     | Yüksek       | Düþük     |
| Taþýnabilirlik   | Yüksek       | Düþük     |
| Okunabilirlik    | Orta         | Yüksek    |
| STM32 HAL uyumu  | Tam          | Yok       |
| Donaným register | **Önerilir** | ?         |


STM32 Ýçin Altýn Kural:
	-Donaným register eriþimi › MASKE
	-Konfigürasyon / yazýlým içi yapý › Bit-field olabilir
	-Register tanýmý yaparken bit-field kullanma
	-Bit-field sadece yazýlým içi state için düþünülebilir

*/































