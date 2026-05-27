#include <stdint.h>
#include <stdio.h>
#include <stddef.h> /* NULL icin */

/*
 * Hata Yonetimi
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule  8.7  — sadece bir dosyadan erisilen nesne static olmali
 *   Rule 14.4  — if kosulu esansiyel boolean tipinde olmali
 *   Rule 15.5  — fonksiyonun tek return noktasi olmali
 *   Rule 17.7  — non-void fonksiyon donus degeri kullanilmali
 *
 * Konu:
 *   Gomulu sistemde hata yonetimi uc katmandan olusur:
 *     1) Hata kodlari    — fonksiyonlar hata durumunu donus degeriyle bildirir
 *     2) Assert          — gelistirme sirasinda gecersiz durumu yakalar
 *     3) Fault handler   — donanim hatasini yakalar, bilgi toplar, resetler
 */

/* PROTOTIP BILDIRIMLERI */

static void hata_kodu_ornegi(void);
static void assert_ornegi(void);
static void fault_handler_ornegi(void);
static void hata_loglama(void);
static void yaygin_hatalar(void);

/* HATA KODU TANIMLARI */

/* Hata kodlari — her modul kendi araligini kullanir, catisma onlenir    */
typedef enum {
    HATA_YOK          = 0x00U, /* basari — islem tamamlandi              */

    /* Genel hatalar — 0x01..0x0F */
    HATA_NULL_PTR     = 0x01U, /* NULL pointer hatasi                    */
    HATA_ARALIK_DISI  = 0x02U, /* deger gecerli aralik disinda            */
    HATA_ZAMAN_ASIMI  = 0x03U, /* islem zaman asimina ugradi             */
    HATA_BOS          = 0x04U, /* kaynak bos — okunacak veri yok         */
    HATA_DOLU         = 0x05U, /* kaynak dolu — yazma yapilamaz          */
    HATA_GECERSIZ     = 0x06U, /* gecersiz parametre veya durum          */

    /* UART hatalari — 0x10..0x1F */
    HATA_UART_FRAME   = 0x10U, /* cerceve hatasi                         */
    HATA_UART_PARITE  = 0x11U, /* parite hatasi                          */
    HATA_UART_OVERFLOW= 0x12U, /* overrun — tampon dolmadan veri geldi   */

    /* SPI hatalari — 0x20..0x2F */
    HATA_SPI_BUS      = 0x20U, /* bus hatasi                             */
    HATA_SPI_CS       = 0x21U, /* chip select hatasi                     */

    /* ADC hatalari — 0x30..0x3F */
    HATA_ADC_KALIBR   = 0x30U, /* kalibrasyon hatasi                     */
    HATA_ADC_DÖNÜS    = 0x31U, /* cevrim zaman asimi                     */

    /* Flash hatalari — 0x40..0x4F */
    HATA_FLASH_YAZ    = 0x40U, /* yazma hatasi                           */
    HATA_FLASH_SIL    = 0x41U, /* silme hatasi                           */
    HATA_FLASH_CRC    = 0x42U  /* CRC dogrulama hatasi                   */
} HataKod_t;

/* Hata bilgisi yapisi — hata koduna ek olarak konum ve zaman bilgisi    */
typedef struct {
    HataKod_t kod;          /* hata kodu                                 */
    uint32_t  satir;        /* hatanin olustugu satir numarasi            */
    uint8_t   dosya[16U];   /* hatanin olustugu dosya adi                */
    uint32_t  tick;         /* hatanin olustugu sistem zamani — ms       */
    uint32_t  ekstra;       /* hata spesifik ek bilgi                    */
} HataBilgi_t;

/* HATA KAYIT TAMPONU */

/* Son N hatayi sakla — debug ve saha analizi icin                       */
#define HATA_LOG_MAX   (8U)  /* kac hata saklaniyor — dairesel tampon    */

static HataBilgi_t g_hata_log[HATA_LOG_MAX]; /* hata kayit tamponu       */
static uint8_t     g_hata_log_idx  = 0U;     /* bir sonraki yazma indeksi */
static uint32_t    g_hata_sayisi   = 0U;     /* toplam hata sayisi        */
static uint32_t    g_sim_tick      = 0U;     /* simule edilmis sistem zamani */

/* ASSERT TANIMI */

/* Assert — gelistirme sirasinda gecersiz durumu erken yakalar           */
/* Release buildde assert tamamen kaldirilir — sifir maliyet             */
#ifdef DEBUG_AKTIF
    /* Debug buildde: kosul saglanmazsa hata handler cagrilir            */
    #define ASSERT(kosul)  \
        do { \
            if ((kosul) == 0U) { \
                assert_handler(__LINE__, "Assert basarisiz"); \
            } \
            else { \
                /* kosul saglandı */ \
            } \
        } while (0)
#else
    /* Release buildde: assert tamamen kaldirilir                        */
    #define ASSERT(kosul)  ((void)(kosul))
#endif

/* DEBUG_AKTIF tanimla — bu dosyada assert aktif olsun                   */
#define DEBUG_AKTIF

/* Assert handler — assert basarisiz oldugunda cagrilir                  */
static void assert_handler(uint32_t satir, const uint8_t * const mesaj)
{
    /* Gercek projede:                                                    */
    /* — hatayi kalici bellegе yaz (Flash/EEPROM)                       */
    /* — watchdog beslemeyi durdur — kontroll reset                      */
    /* — UART ile hata bilgisini gonder                                  */

    printf("  ASSERT HATASI   : satir=%u mesaj=%s\n",
           (uint32_t)satir, mesaj);

    /* Sonsuz dongu — debugger ile yakalamak icin                        */
    /* Gercek projede watchdog burada sistemi resetler                   */
    /* while(1U) { } — simülasyonda yorum icinde birakıldı              */
}

/* HATA LOGLAMA FONKSİYONLARI */

/* Hata kaydet — dairesel tampona yaz                                    */
static void hata_kaydet(HataKod_t kod, uint32_t satir,
                         const uint8_t * const dosya, uint32_t ekstra)
{
    uint8_t i;

    /* Dairesel tampon — eski kayitlarin uzerine yaz                     */
    g_hata_log[g_hata_log_idx].kod   = kod;
    g_hata_log[g_hata_log_idx].satir = satir;
    g_hata_log[g_hata_log_idx].tick  = g_sim_tick;
    g_hata_log[g_hata_log_idx].ekstra = ekstra;

    /* Dosya adini kopyala — string.h kullanmadan, MISRA uyumlu          */
    for (i = 0U; i < 15U; i++)
    {
        if (dosya[i] == (uint8_t)'\0') /* string sonu                    */
        {
            g_hata_log[g_hata_log_idx].dosya[i] = (uint8_t)'\0';
            break;
        }
        else
        {
            g_hata_log[g_hata_log_idx].dosya[i] = dosya[i];
        }
    }
    g_hata_log[g_hata_log_idx].dosya[15U] = (uint8_t)'\0'; /* null terminator */

    /* Bir sonraki indekse gec — tasinrsa basa don                       */
    g_hata_log_idx = (uint8_t)((g_hata_log_idx + 1U) % HATA_LOG_MAX);
    g_hata_sayisi++;
}

/* Hata logunu yazdir — son kayitlari goster                             */
static void hata_log_yazdir(void)
{
    uint8_t   i;
    uint8_t   adet = (g_hata_sayisi < (uint32_t)HATA_LOG_MAX) ?
                     (uint8_t)g_hata_sayisi : HATA_LOG_MAX;

    printf("  Toplam hata     : %u\n", (uint32_t)g_hata_sayisi);

    for (i = 0U; i < adet; i++)
    {
        printf("  Log[%u]          : kod=0x%02X satir=%-4u tick=%-6u ekstra=0x%04X\n",
               (uint32_t)i,
               (uint32_t)g_hata_log[i].kod,
               (uint32_t)g_hata_log[i].satir,
               (uint32_t)g_hata_log[i].tick,
               (uint32_t)g_hata_log[i].ekstra);
    }
}

/* HATA KODU ORNEGI */

/* UART veri gonder — hata kodu donduren ornek fonksiyon                 */
static HataKod_t uart_gonder(const uint8_t * const veri, uint8_t boy)
{
    HataKod_t ret;

    /* Parametre dogrulama — fonksiyon girisinde kontrol                 */
    if (veri == NULL) /* NULL pointer kontrolu                           */
    {
        ret = HATA_NULL_PTR;
    }
    else if (boy == 0U) /* gecersiz boy                                   */
    {
        ret = HATA_ARALIK_DISI;
    }
    else if (boy > 64U) /* maksimum desteklenen boy                       */
    {
        ret = HATA_ARALIK_DISI;
    }
    else
    {
        /* STM32 HAL kodu:                                                */
        /* HAL_StatusTypeDef durum = HAL_UART_Transmit(                  */
        /*     &huart1, veri, boy, 100U);                                */
        /* if (durum != HAL_OK) { return HATA_ZAMAN_ASIMI; }            */

        printf("  UART gonder     : %u byte gonderildi\n", (uint32_t)boy);
        ret = HATA_YOK; /* basarili                                       */
    }

    return ret; /* tek return noktasi                                     */
}

/* ADC oku — zincirleme hata yonetimi ornegi                             */
static HataKod_t adc_oku(uint16_t * const sonuc_out)
{
    HataKod_t ret;

    if (sonuc_out == NULL) /* cikis pointer kontrolu                      */
    {
        ret = HATA_NULL_PTR;
    }
    else
    {
        /* STM32 HAL kodu:                                                */
        /* HAL_ADC_Start(&hadc1);                                        */
        /* if (HAL_ADC_PollForConversion(&hadc1, 10U) != HAL_OK)        */
        /* { return HATA_ADC_DÖNÜS; }                                   */
        /* *sonuc_out = HAL_ADC_GetValue(&hadc1);                        */

        *sonuc_out = 2048U; /* simule edilmis ADC degeri                  */
        ret = HATA_YOK;
    }

    return ret;
}

static void hata_kodu_ornegi(void)
{
    HataKod_t hata;
    uint8_t   veri[4U] = {0xAAU, 0xBBU, 0xCCU, 0xDDU};
    uint16_t  adc_val  = 0U;

    /* Basarili islem */
    hata = uart_gonder(veri, 4U);
    if (hata != HATA_YOK) /* donus degeri her zaman kontrol edilmeli     */
    {
        hata_kaydet(hata, (uint32_t)__LINE__,
                    (const uint8_t *)"HataYonetimi", 0U);
        printf("  UART hatasi     : kod=0x%02X\n", (uint32_t)hata);
    }
    else
    {
        printf("  UART basarili   : hata yok\n");
    }

    /* NULL pointer hatasi */
    hata = uart_gonder(NULL, 4U);
    if (hata != HATA_YOK)
    {
        hata_kaydet(hata, (uint32_t)__LINE__,
                    (const uint8_t *)"HataYonetimi", 0U);
        printf("  NULL ptr hatasi : kod=0x%02X\n", (uint32_t)hata);
    }
    else
    {
        /* hata yok */
    }

    /* Aralik disi hatasi */
    hata = uart_gonder(veri, 0U);
    if (hata != HATA_YOK)
    {
        hata_kaydet(hata, (uint32_t)__LINE__,
                    (const uint8_t *)"HataYonetimi", 0U);
        printf("  Aralik hatasi   : kod=0x%02X\n", (uint32_t)hata);
    }
    else
    {
        /* hata yok */
    }

    /* ADC okuma */
    g_sim_tick += 100U;
    hata = adc_oku(&adc_val);
    if (hata != HATA_YOK)
    {
        hata_kaydet(hata, (uint32_t)__LINE__,
                    (const uint8_t *)"HataYonetimi", 0U);
        printf("  ADC hatasi      : kod=0x%02X\n", (uint32_t)hata);
    }
    else
    {
        printf("  ADC basarili    : deger=%u\n", (uint32_t)adc_val);
    }
}

/* ASSERT ORNEGI */

static void assert_ornegi(void)
{
    uint8_t  boy    = 10U;
    uint8_t *ptr    = NULL;
    uint8_t  deger  = 5U;
    uint8_t  buf[8U] = {0U};

    /* Gecerli assert — kosul saglanir, devam eder                       */
    ASSERT(boy > 0U);
    printf("  Assert gecti    : boy > 0\n");

    /* Pointer kontrolu — NULL ise assert tetiklenir                     */
    ptr = buf; /* gecerli pointer                                        */
    ASSERT(ptr != NULL);
    printf("  Assert gecti    : ptr != NULL\n");

    /* Aralik kontrolu */
    ASSERT((deger >= 0U) && (deger <= 100U));
    printf("  Assert gecti    : deger aralik icinde\n");

    /* Assert ne zaman kullanilmali:                                      */
    /* — Programlama hatalarini yakalamak icin (yanlis cagri sirasi)     */
    /* — Calisma zamanı hatalari icin degil (sensor verisi, kullanici)  */
    /* — Release buildde kaldırılır — runtime kontrolu gerekenler        */
    /*   if/else ile ele alinmali                                        */
    printf("  Assert ozeti    : gelistirme hatasi icin, runtime icin degil\n");
}

/* FAULT HANDLER */

/* HardFault bilgi yapisi — Cortex-M registerlarini yakalar              */
typedef struct {
    uint32_t r0;    /* genel amacli register                             */
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;    /* link register — hangi fonksiyondan geldi          */
    uint32_t pc;    /* program counter — hata adresi                    */
    uint32_t psr;   /* program status register                           */
} FaultFrame_t;

/* HardFault bilgisini sakla — reset sonrasi okunabilir alan             */
/* STM32'de bu yapi RTC backup registerda veya Flash'in ozel bolumunde  */
/* saklanir — RAM Standby modunda kaybolur                               */
static FaultFrame_t g_fault_bilgi; /* simülasyon icin RAM'de            */
static uint8_t      g_fault_olustu = 0U;

/* HardFault handler — gercek STM32 assembly ile baslar                  */
static void HardFault_Handler_sim(uint32_t pc_adresi)
{
    /* Gercek STM32 kodu (stm32fXxx_it.c):                               */
    /*                                                                    */
    /* void HardFault_Handler(void) {                                    */
    /*   __asm volatile (                                                 */
    /*     "TST lr, #4      \n"  — hangi stack kullaniliyor?             */
    /*     "ITE EQ          \n"                                          */
    /*     "MRSEQ r0, MSP   \n"  — Main Stack Pointer                   */
    /*     "MRSNE r0, PSP   \n"  — Process Stack Pointer                */
    /*     "B fault_handler \n"                                          */
    /*   );                                                              */
    /* }                                                                  */
    /*                                                                    */
    /* void fault_handler(uint32_t *frame) {                             */
    /*   g_fault_bilgi.r0  = frame[0];                                   */
    /*   g_fault_bilgi.pc  = frame[6]; — hata adresi                    */
    /*   g_fault_bilgi.lr  = frame[5];                                   */
    /*   — bilgileri kalici bellegе yaz                                  */
    /*   — sistemi resetle                                                */
    /* }                                                                  */

    g_fault_bilgi.pc = pc_adresi; /* hatayi olusturan adres              */
    g_fault_bilgi.lr = 0x08001234U; /* cagiran fonksiyon adresi          */
    g_fault_olustu   = 1U;

    printf("  HardFault       : PC=0x%08X LR=0x%08X\n",
           (uint32_t)g_fault_bilgi.pc,
           (uint32_t)g_fault_bilgi.lr);
    printf("  Neden olabilir  : NULL deref, yanlis hizalama, yetki ihlali\n");
}

/* Sistem basladiginda onceki fault kontrolu                             */
static void fault_kontrol_et(void)
{
    if (g_fault_olustu == 1U) /* onceki calisımda fault oldu mu?         */
    {
        printf("  Onceki fault    : PC=0x%08X\n", (uint32_t)g_fault_bilgi.pc);
        printf("  Aksiyon         : hata loguna yaz, kullaniciya bildir\n");
        g_fault_olustu = 0U; /* bayragi temizle                           */
    }
    else
    {
        printf("  Fault kontrol   : temiz baslamaع\n");
    }
}

static void fault_handler_ornegi(void)
{
    /* Normal baslama — onceki fault yok                                  */
    fault_kontrol_et();

    /* Fault simule et */
    printf("  Fault simule    :\n");
    HardFault_Handler_sim(0x080034A8U); /* hata adresi                   */

    /* Sonraki "baslama" simulasyonu */
    fault_kontrol_et();
}

/* HATA LOGLAMA */

static void hata_loglama(void)
{
    /* Birkac hata uret — loglama sistemini test et                      */
    g_sim_tick += 500U;
    hata_kaydet(HATA_UART_FRAME,   10U, (const uint8_t *)"uart.c",    0x00U);

    g_sim_tick += 200U;
    hata_kaydet(HATA_ADC_DÖNÜS,   45U, (const uint8_t *)"adc.c",     0x01U);

    g_sim_tick += 100U;
    hata_kaydet(HATA_FLASH_CRC,   89U, (const uint8_t *)"flash.c",   0xDEADU);

    g_sim_tick += 300U;
    hata_kaydet(HATA_ZAMAN_ASIMI, 23U, (const uint8_t *)"sensor.c",  0x00U);

    /* Hata logunu yazdir */
    printf("Hata log icerigi  :\n");
    hata_log_yazdir();
}

/* YAYGIN HATALAR */

static void yaygin_hatalar(void)
{
    /* 1) Donus degeri kontrol edilmemesi — sessiz hata                  */
    {
        /* uart_gonder(veri, 4U); — YANLIS: donus degeri yoksayildi      */
        /* Hata olsa bile program devam eder — veri gonderilmedi         */
        /* Cozum: Rule 17.7 — non-void donus degeri kullanilmali         */
        printf("Donus degerı      : her zaman kontrol et (Rule 17.7)\n");
    }

    /* 2) Hata kodunu yukar iletmemek — kayıp bilgi                      */
    {
        /* HataKod_t alt_fonk(void) { return HATA_ZAMAN_ASIMI; }        */
        /* void ust_fonk(void) { alt_fonk(); } — hata yutuldu           */
        /* Cozum: hata kodu zinciri boyunca iletilmeli                   */
        printf("Hata zinciri      : hatayi yukari ilet, yutma\n");
    }

    /* 3) Assert'i runtime hata icin kullanmak                           */
    {
        /* ASSERT(sensor_oku() == HATA_YOK); — YANLIS                   */
        /* Sensor okuma basarisizligi programlama hatasi degil           */
        /* Assert sadece "olmamasi gereken" durumlar icin                */
        /* Cozum: runtime hatalari if/else ile yonet                    */
        printf("Assert kapsami    : programlama hatasi icin, runtime icin degil\n");
    }

    /* 4) Hata handler'da uzun islem                                     */
    {
        /* HardFault handler icinde printf, malloc vb. kullamma          */
        /* Stack cokmuş olabilir — minimal islem yap, resetle            */
        /* Cozum: sadece kritik bilgiyi kaydet, watchdog ile resetle     */
        printf("Fault handler     : minimal islem yap, hemen resetle\n");
    }
}

/* MAIN */

int main(void)
{
    printf("/* HATA KODU ORNEGI */\n");
    hata_kodu_ornegi();

    printf("\n/* ASSERT ORNEGI */\n");
    assert_ornegi();

    printf("\n/* FAULT HANDLER */\n");
    fault_handler_ornegi();

    printf("\n/* HATA LOGLAMA */\n");
    hata_loglama();

    printf("\n/* YAYGIN HATALAR */\n");
    yaygin_hatalar();

    return 0;
}