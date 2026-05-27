#include <stdint.h>
#include <stdio.h>

/*
 * Power Management
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule  8.7  — sadece bir dosyadan erisilen nesne static olmali
 *   Rule 14.4  — if kosulu esansiyel boolean tipinde olmali
 *   Rule 15.5  — fonksiyonun tek return noktasi olmali
 *
 * Platform notu:
 *   STM32 spesifik HAL fonksiyonlari ve registerlar yorum icinde
 *   gosterilmistir. PC ortaminda simule edilmis karsiliklari calisir.
 *
 * Konu:
 *   STM32'de bes uyku modu bulunur:
 *     1) Sleep       — CPU durur, peripheraller calisir
 *     2) Low-Power Sleep — Sleep + voltaj dusurilur
 *     3) Stop 0/1    — CPU + cogu peripheral durur, RAM korunur
 *     2) Stop 2      — daha derin, daha az peripheral aktif
 *     5) Standby     — neredeyse her sey kapanir, RAM kaybolur
 *     6) Shutdown    — en derin mod, sadece VBAT domaini aktif
 */

/* PROTOTIP BILDIRIMLERI */

static void uyku_modlari(void);
static void wakeup_kaynaklari(void);
static void saat_yonetimi(void);
static void peripheral_guc_yonetimi(void);
static void guc_tuketim_olcumu(void);
static void yaygin_hatalar(void);

/* SABIT TANIMLAR */

/* Uyku modu tanimlari — STM32L4 referans alinmistir                     */
typedef enum {
    GUC_MOD_RUN       = 0U, /* tam performans — tum birimler aktif       */
    GUC_MOD_SLEEP     = 1U, /* CPU durur — peripheraller calisir         */
    GUC_MOD_LP_SLEEP  = 2U, /* dusuk voltaj sleep                        */
    GUC_MOD_STOP0     = 3U, /* cogu peripheral durur — RAM korunur       */
    GUC_MOD_STOP1     = 4U, /* daha az regulator aktif                   */
    GUC_MOD_STOP2     = 5U, /* en az regulator — en dusuk Stop tuketime  */
    GUC_MOD_STANDBY   = 6U, /* RAM kaybolur — sadece RTC/WKUP aktif     */
    GUC_MOD_SHUTDOWN  = 7U  /* en derin — sadece VBAT domaini            */
} GucMod_t;

/* Wakeup kaynagi bitleri — PWR_CR3 / PWR_WUSR register bitleri          */
#define WAKEUP_PIN1       (1U << 0U) /* PA0  — wakeup pin 1              */
#define WAKEUP_PIN2       (1U << 1U) /* PC13 — wakeup pin 2              */
#define WAKEUP_RTC        (1U << 3U) /* RTC alarm veya wakeup timer      */
#define WAKEUP_LPUART     (1U << 5U) /* LPUART1 — dusuk guc UART         */
#define WAKEUP_COMP       (1U << 6U) /* comparator cikisi                */

/* Saat hizi sabitleri — STM32L4 */
#define SAAT_80MHZ        (80000000U) /* maksimum — tam performans        */
#define SAAT_24MHZ        (24000000U) /* orta performans — dusuk tuketim  */
#define SAAT_4MHZ         (4000000U)  /* dusuk performans — bekleme modu  */
#define SAAT_100KHZ       (100000U)   /* minimum — cok derin bekleme      */

/* Tipik guc tuketim degerleri — STM32L476 datasheet (uA)                */
#define TUKETIM_RUN_MA    (10U)  /* Run mod  — 80MHz, tum peripheral     */
#define TUKETIM_SLEEP_MA  (2U)   /* Sleep    — CPU durmus                */
#define TUKETIM_STOP_UA   (10U)  /* Stop 2   — uA cinsinden              */
#define TUKETIM_STBY_UA   (1U)   /* Standby  — uA cinsinden              */

/* SIMULASYON DEGISKENLERI */

static GucMod_t  g_guncel_mod     = GUC_MOD_RUN; /* mevcut guc modu      */
static uint32_t  g_saat_hizi      = SAAT_80MHZ;  /* mevcut CPU saati     */
static uint8_t   g_wakeup_kaynagi = 0U;           /* son wakeup kaynagi   */
static uint32_t  g_uyku_sure_ms   = 0U;           /* toplam uyku suresi   */

/* GUC MODU GECIS FONKSİYONLARI */

/* Sleep moduna gec — CPU WFI/WFE komutu ile durdurulur                  */
static void sleep_gir(void)
{
    /* STM32 HAL kodu:                                                    */
    /* HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI); */
    /*                                                                    */
    /* Register seviyesi:                                                 */
    /* SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk; — deep sleep bitti           */
    /* __WFI(); — Wait For Interrupt — kesme gelene kadar bekle          */

    g_guncel_mod = GUC_MOD_SLEEP;
    printf("  Sleep girildi   : CPU durdu, peripheral aktif\n");
    printf("  Tuketim         : ~%u mA\n", (uint32_t)TUKETIM_SLEEP_MA);
}

/* Stop 2 moduna gec — en dusuk tuketimli Stop modu                      */
static void stop2_gir(uint32_t sure_ms)
{
    /* STM32 HAL kodu:                                                    */
    /* HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);                      */
    /*                                                                    */
    /* Register seviyesi:                                                 */
    /* PWR->CR1 |= PWR_CR1_LPMS_STOP2; — Stop 2 mod sec                 */
    /* SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk; — deep sleep aktif            */
    /* __WFI();                                                           */
    /*                                                                    */
    /* Stop 2 cikisinda:                                                  */
    /* SystemClock_Config(); — saat yeniden yapilandirilmali             */

    g_guncel_mod  = GUC_MOD_STOP2;
    g_uyku_sure_ms += sure_ms;
    printf("  Stop2 girildi   : cogu peripheral durdu, RAM korundu\n");
    printf("  Uyku suresi     : %u ms\n", (uint32_t)sure_ms);
    printf("  Tuketim         : ~%u uA\n", (uint32_t)TUKETIM_STOP_UA);
}

/* Standby moduna gec — RAM kaybolur, sadece RTC ve wakeup pini aktif    */
static void standby_gir(uint32_t sure_ms)
{
    /* STM32 HAL kodu:                                                    */
    /* HAL_PWR_EnterSTANDBYMode();                                        */
    /*                                                                    */
    /* Register seviyesi:                                                 */
    /* PWR->CR1 |= PWR_CR1_LPMS_STANDBY;                                 */
    /* SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;                                */
    /* __WFI();                                                           */
    /*                                                                    */
    /* Standby cikisinda sistem RESETLENIR — main() basından baslar      */
    /* RAM verileri kaybolur — kalici veri RTC backup registerda saklanir */

    g_guncel_mod  = GUC_MOD_STANDBY;
    g_uyku_sure_ms += sure_ms;
    printf("  Standby girildi : RAM kaybolur — wakeup = reset gibi\n");
    printf("  Uyku suresi     : %u ms\n", (uint32_t)sure_ms);
    printf("  Tuketim         : ~%u uA\n", (uint32_t)TUKETIM_STBY_UA);
}

/* Uyku modundan don — saat ve peripheral yapilandirmasi                  */
static void uyku_don(void)
{
    /* Stop/Standby cikisinda saat yapilandirmasi gerekir                 */
    /* STM32 HAL kodu:                                                    */
    /* SystemClock_Config(); — PLL yeniden ayarla                        */
    /* HAL_PWREx_DisableLowPowerRunMode(); — LP run mod kapat            */

    g_guncel_mod = GUC_MOD_RUN;
    g_saat_hizi  = SAAT_80MHZ; /* saat yeniden maksimuma ayarlandi        */
    printf("  Uyku sonu       : saat yeniden yapilandirildi\n");
    printf("  Mod             : RUN — %u Hz\n", (uint32_t)g_saat_hizi);
}

/* UYKU MODLARI */

static void uyku_modlari(void)
{
    printf("Mevcut mod        : RUN — %u Hz\n", (uint32_t)g_saat_hizi);
    printf("Tuketim           : ~%u mA\n\n", (uint32_t)TUKETIM_RUN_MA);

    /* Sleep modu — peripheraller calisirken CPU bekler                  */
    /* Kullanim: hizli wakeup gerekli, UART/SPI aktif olmali             */
    printf("Sleep modu        :\n");
    sleep_gir();
    uyku_don();

    printf("\n");

    /* Stop 2 modu — uzun bekleme, periyodik wakeup                      */
    /* Kullanim: sensor dugumu, periyodik olcum, RTC ile uyan            */
    printf("Stop2 modu        :\n");
    stop2_gir(5000U); /* 5 saniye uyu                                     */
    uyku_don();

    printf("\n");

    /* Standby modu — en uzun bekleme, batarya kritik                    */
    /* Kullanim: pil tasarruf modu, sadece wakeup pini ile uyanma        */
    printf("Standby modu      :\n");
    standby_gir(30000U); /* 30 saniye uyu                                 */
    /* Gercekte buradan devam edilmez — sistem resetlenir                */
    uyku_don(); /* simulasyon icin devam ettiriliyor                      */

    printf("\nToplam uyku       : %u ms\n", (uint32_t)g_uyku_sure_ms);
}

/* WAKEUP KAYNAKLARI */

static void wakeup_kaynaklari(void)
{
    printf("Wakeup kaynaklari :\n");

    /* RTC wakeup — periyodik zamanlayici ile uyanma                     */
    {
        /* STM32 HAL kodu:                                                */
        /* HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 10, RTC_WAKEUPCLOCK_CK_SPRE_16BITS); */
        /* — 10 saniyede bir wakeup kesmesi uret                         */

        printf("  RTC wakeup      : periyodik olcum icin ideal\n");
        printf("  Ornek           : her 10 saniyede bir sensor oku\n");

        /* Wakeup sonrasi islem yapilip tekrar uyunabilir                 */
        g_wakeup_kaynagi = WAKEUP_RTC;
    }

    /* Wakeup pini — dis sinyal ile uyanma                               */
    {
        /* STM32 HAL kodu:                                                */
        /* HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1); — PA0 aktif et      */
        /* — rising edge ile wakeup                                      */

        printf("  Wakeup pin      : buton veya dis sinyal ile uyanma\n");
        printf("  Ornek           : kullanici butonu ile ekrani ac\n");
    }

    /* LPUART wakeup — dusuk guclu UART ile uyanma                       */
    {
        /* STM32 HAL kodu:                                                */
        /* HAL_UARTEx_EnableStopMode(&hlpuart1);                         */
        /* — Stop modunda UART veri aliminda uyan                        */

        printf("  LPUART wakeup   : UART komutu aliminda uyanma\n");
        printf("  Ornek           : kablosuz modülden komut bekle\n");
    }

    /* Wakeup kaynagi tespiti — hangi kaynak uyandirdi?                  */
    {
        /* STM32 HAL kodu:                                                */
        /* if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUF3)) — RTC mi?             */
        /* if (__HAL_PWR_GET_FLAG(PWR_FLAG_WUF1)) — pin mi?             */
        /* __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF); — bayraklari temizle     */

        if ((g_wakeup_kaynagi & WAKEUP_RTC) != 0U)
        {
            printf("  Son wakeup      : RTC alarm\n");
        }
        else if ((g_wakeup_kaynagi & WAKEUP_PIN1) != 0U)
        {
            printf("  Son wakeup      : wakeup pin 1\n");
        }
        else
        {
            printf("  Son wakeup      : bilinmiyor\n");
        }
    }
}

/* SAAT YONETIMI */

/* Dinamik saat olcekleme — is yukune gore saat hizini ayarla            */
static void saat_ayarla(uint32_t hedef_hz)
{
    /* STM32 HAL kodu:                                                    */
    /* RCC_OscInitTypeDef osc = {0};                                      */
    /* RCC_ClkInitTypeDef clk = {0};                                      */
    /* — PLL carpanlarini degistir, flash gecikme surelerini ayarla      */
    /* HAL_RCC_OscConfig(&osc);                                           */
    /* HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_X);                        */

    g_saat_hizi = hedef_hz;
    printf("  Saat ayarlandi  : %u Hz\n", (uint32_t)hedef_hz);
}

static void saat_yonetimi(void)
{
    printf("Dinamik saat olcekleme:\n");

    /* Yogun islem — maksimum hiz                                         */
    saat_ayarla(SAAT_80MHZ);
    printf("  Yogun islem     : FFT, sifrelem vb.\n");

    /* Normal operasyon — orta hiz, dusuk tuketim                        */
    saat_ayarla(SAAT_24MHZ);
    printf("  Normal islem    : sensor okuma, protokol isleme\n");

    /* Bekleme — minimum hiz                                              */
    saat_ayarla(SAAT_4MHZ);
    printf("  Bekleme         : kullanici girisi bekleniyor\n");

    /* MSI (Multi-Speed Internal) osilatoru — STM32L4 ozelligi           */
    /* STM32 HAL kodu:                                                    */
    /* __HAL_RCC_MSI_RANGE_CONFIG(RCC_MSIRANGE_3); — 800 kHz             */
    /* — en dusuk dinamik tuketim icin kullan                            */
    saat_ayarla(SAAT_100KHZ);
    printf("  Minimum hiz     : sadece RTC ve LPUART aktif\n");
}

/* PERIPHERAL GUC YONETIMI */

static void peripheral_guc_yonetimi(void)
{
    printf("Peripheral guc yonetimi:\n");

    /* Kullanilmayan peripheral saatlerini kapat — AHB/APB bus saatleri  */
    {
        /* STM32 HAL kodu:                                                */
        /* __HAL_RCC_GPIOA_CLK_DISABLE(); — kullanilmayan GPIO saati kapat */
        /* __HAL_RCC_SPI1_CLK_DISABLE();  — kullanilmayan SPI saati kapat  */
        /* __HAL_RCC_ADC_CLK_DISABLE();   — ADC saati kapat              */

        printf("  Saat kapatma    : kullanilmayan peripheral saati kapat\n");
        printf("  Tasarruf        : her peripheral ~0.1-1 mA tasarruf\n");
    }

    /* GPIO analog moda al — floating input en cok tuketen               */
    {
        /* STM32 HAL kodu:                                                */
        /* GPIO_InitTypeDef gpio = {0};                                   */
        /* gpio.Mode  = GPIO_MODE_ANALOG;                                 */
        /* gpio.Pull  = GPIO_NOPULL;                                      */
        /* gpio.Pin   = GPIO_PIN_All; — tum pinler                       */
        /* HAL_GPIO_Init(GPIOA, &gpio); — kullanilmayan portlari analog  */

        printf("  GPIO analog     : kullanilmayan pinleri analog moda al\n");
        printf("  Tasarruf        : floating input ~1 uA/pin tasarruf\n");
    }

    /* ADC auto-off modu — cevrim sonrasi otomatik kapanir               */
    {
        /* STM32 HAL kodu:                                                */
        /* hadc1.Init.LowPowerAutoWait   = ENABLE; — otomatik bekleme   */
        /* hadc1.Init.LowPowerAutoPowerOff = ENABLE; — otomatik kapanma */

        printf("  ADC auto-off    : cevrim sonrasi ADC kapanir\n");
    }

    /* Flash guc tasarrufu — okuma tampon ve on-bellek                   */
    {
        /* STM32 HAL kodu:                                                */
        /* __HAL_FLASH_SLEEP_POWERDOWN_ENABLE(); — sleep'te flash off    */

        printf("  Flash sleep-off : sleep modunda flash kapanir\n");
    }
}

/* GUC TUKETIM OLCUMU */

/* Farkli senaryolarin tahmini pil omru hesabi                           */
static void guc_tuketim_olcumu(void)
{
    /* Batarya kapasitesi — ornek: CR2032 pil = 225 mAh                  */
    uint32_t batarya_mah    = 225U;

    /* Senaryo 1: her zaman RUN mod                                       */
    {
        /* Ortalama tuketim: 10 mA                                       */
        uint32_t tuketim_ma  = 10U;
        uint32_t omur_saat   = batarya_mah / tuketim_ma; /* 22.5 saat   */
        printf("Senaryo 1 (daima RUN): ~%u saat\n", (uint32_t)omur_saat);
    }

    /* Senaryo 2: duty cycle — 10ms RUN, 990ms Stop2                     */
    {
        /* Ortalama tuketim:                                              */
        /* (10ms * 10mA + 990ms * 0.01mA) / 1000ms                      */
        /* = (100 + 9.9) / 1000 = 0.11 mA ortalama                      */
        uint32_t ort_tuketim_ua = 110U; /* uA cinsinden                  */
        uint32_t omur_saat = (batarya_mah * 1000U) / ort_tuketim_ua;
        printf("Senaryo 2 (duty cycle): ~%u saat (~%u gun)\n",
               (uint32_t)omur_saat,
               (uint32_t)(omur_saat / 24U)); /* 2045 saat ~ 85 gun      */
    }

    /* Senaryo 3: 1 saniye Stop2, 10ms RUN — IoT sensor dugumu           */
    {
        /* Ortalama: (10ms * 10mA + 990ms * 0.01mA) / 1000ms ~ 0.11mA  */
        /* RTC wakeup + olcum + iletim + Stop2                           */
        uint32_t ort_tuketim_ua = 50U; /* agresif optimizasyon ile       */
        uint32_t omur_saat = (batarya_mah * 1000U) / ort_tuketim_ua;
        printf("Senaryo 3 (IoT sensor): ~%u saat (~%u gun)\n",
               (uint32_t)omur_saat,
               (uint32_t)(omur_saat / 24U)); /* ~4500 saat ~ 187 gun    */
    }
}

/* YAYGIN HATALAR */

static void yaygin_hatalar(void)
{
    /* 1) Stop modundan donerken saat yeniden yapilandirilmamasi         */
    {
        /* Stop modunda PLL kapanir — cikista saat MSI'ya duser          */
        /* HAL fonksiyonlari yanlis hizda calisir — UART baud hatalı     */
        /* Cozum: Stop cikisi kesmesinde SystemClock_Config() cagir      */
        printf("Saat yapilandir  : Stop cikisinda SystemClock_Config()\n");
    }

    /* 2) Standby oncesi RAM verisi kaybolacak                            */
    {
        /* Standby'da SRAM kaybolur — kritik veri backup registerlara   */
        /* yazilmali: RTC_BKPxR — 32 adet 32-bit register               */
        /* STM32 HAL: HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, deger);  */
        printf("Standby RAM      : kritik veriyi RTC backup'a yaz\n");
    }

    /* 3) Wakeup bayragini temizlememek — tekrar tekrar wakeup           */
    {
        /* Wakeup bayragi temizlenmezse sistem uyuyamaz                  */
        /* STM32 HAL: __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF);               */
        printf("Wakeup bayragi   : uyku oncesi bayraklari temizle\n");
    }

    /* 4) Debug modunda uyku — baglanti kesilir                          */
    {
        /* Stop/Standby modunda SWD baglantisi kesilir                   */
        /* Debugger ile uyku modlarini test etmek zorlaşır               */
        /* Cozum: DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP; — debug uyku aktif */
        printf("Debug uyku       : DBGMCU DBG_SLEEP bitini set et\n");
    }

    /* 5) Peripheral durdurulmadan uyku — veri kaybi                     */
    {
        /* UART DMA transfer sirasinda Stop moduna girilirse veri kaybi  */
        /* Cozum: DMA tamamlandi kesmesini bekle, sonra uyku moduna gir  */
        printf("Peripheral dur   : DMA/transfer bitmeden uyuma\n");
    }
}

/* MAIN */

int main(void)
{
    printf("/* UYKU MODLARI */\n");
    uyku_modlari();

    printf("\n/* WAKEUP KAYNAKLARI */\n");
    wakeup_kaynaklari();

    printf("\n/* SAAT YONETIMI */\n");
    saat_yonetimi();

    printf("\n/* PERIPHERAL GUC YONETIMI */\n");
    peripheral_guc_yonetimi();

    printf("\n/* GUC TUKETIM OLCUMU */\n");
    guc_tuketim_olcumu();

    printf("\n/* YAYGIN HATALAR */\n");
    yaygin_hatalar();

    return 0;
}