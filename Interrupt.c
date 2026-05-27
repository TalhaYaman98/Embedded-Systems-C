#include <stdint.h>
#include <stdio.h>

/*
 * Interrupt
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
 *   Bu dosya PC ortaminda derlenir — STM32 spesifik satirlar yorum icinde
 *   gosterilmistir. Gercek projede stm32fXxx_it.c dosyasina tasınır.
 */

/* PROTOTIP BILDIRIMLERI */

static void isr_flag_yonetimi(void);
static void isr_ring_buffer(void);
static void isr_critical_section(void);
static void isr_zamanlayici(void);
static void yaygin_hatalar(void);

/* SABIT TANIMLAR */

#define RING_MAX         (16U) /* UART ring buffer boyutu — 2'nin kuvveti  */
#define TIMEOUT_MS       (100U) /* flag bekleme zaman asimi                */
#define TICK_RATE_MS     (1U)   /* SysTick periyodu — HAL varsayilan 1ms   */

/* ISR ILE PAYLASILAN DEGISKENLER */

/* volatile zorunlu — derleyici bu degiskenleri optimize etmemeli         */
/* ISR herhangi bir anda degeri degistirebilir, main() her okumada        */
/* RAM'den okumayi garanti altina almak icin volatile kullanilir          */
static volatile uint8_t  g_uart_rx_flag   = 0U; /* UART veri alindi isareti    */
static volatile uint8_t  g_tim_flag       = 0U; /* timer periyot isareti       */
static volatile uint8_t  g_hata_flag      = 0U; /* hata durum isareti          */
static volatile uint32_t g_systick_ms     = 0U; /* sistem zamani — ms cinsinden */
static volatile uint32_t g_tim_sayac      = 0U; /* timer kesme sayaci           */

/* UART RING BUFFER — ISR ile main() arasinda veri tasir */

/* volatile struct — ISR tarafindan yazilan, main() tarafindan okunan     */
typedef struct {
    uint8_t          buf[RING_MAX]; /* veri tamponu                        */
    volatile uint8_t bas;           /* okuma indeksi — main() gunceller    */
    volatile uint8_t son;           /* yazma indeksi — ISR gunceller       */
    volatile uint8_t dolu;          /* eleman sayisi — her iki taraf okur  */
} UartRingBuf_t;

static UartRingBuf_t g_uart_buf = {{0U}, 0U, 0U, 0U};

/* ISR SIMULASYONU — PC ortaminda donanim kesmesi olmadigi icin           */
/* gercek STM32 projesinde bu fonksiyonlar asagidaki gibi tanimlanir:     */
/*                                                                         */
/* void USART1_IRQHandler(void) { ... }   — UART kesme servisi rutini    */
/* void TIM2_IRQHandler(void)   { ... }   — Timer kesme servisi rutini   */
/* void SysTick_Handler(void)   { ... }   — SysTick kesme servisi rutini */

static void SysTick_Handler_sim(void)
{
    /* SysTick her 1ms'de bir tetiklenir — HAL_IncTick() buradan cagrilir */
    g_systick_ms++; /* sistem zamani guncelle — volatile oldugu icin guvenli */
}

static void USART1_IRQHandler_sim(uint8_t alinan_byte)
{
    /* UART veri alim kesmesi — her byte geldiginde tetiklenir            */
    /* Gercek STM32 kodu:                                                 */
    /* uint8_t alinan = (uint8_t)(USART1->DR & 0xFFU);                   */

    if (g_uart_buf.dolu < RING_MAX) /* tampon dolu degil — yazilabilir    */
    {
        g_uart_buf.buf[g_uart_buf.son] = alinan_byte;
        /* & ile dairesel indeks — RING_MAX 2'nin kuvveti oldugu icin     */
        g_uart_buf.son   = (uint8_t)((g_uart_buf.son + 1U) & (RING_MAX - 1U));
        g_uart_buf.dolu++;
        g_uart_rx_flag = 1U; /* main()'e veri hazir sinyali ver           */
    }
    else
    {
        g_hata_flag = 1U; /* tampon dolu — veri kaybi, hata isaretlendi   */
    }
}

static void TIM2_IRQHandler_sim(void)
{
    /* Timer periyodik kesmesi — ornegin 10ms'de bir                      */
    /* Gercek STM32 kodu:                                                 */
    /* __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE); — kesme bayragi temizle */

    g_tim_sayac++;     /* kac periyot gectigi sayilir                     */
    g_tim_flag = 1U;   /* main()'e periyot tamamlandi sinyali ver         */
}

/* CRITICAL SECTION SIMULASYONU */

/* STM32'de critical section:                                             */
/* __disable_irq() — tum kesmeleri devre disi birak (PRIMASK = 1)        */
/* __enable_irq()  — kesmeleri yeniden etkinlestir  (PRIMASK = 0)        */
/*                                                                         */
/* Kullanim amaci: cok-byte degisken guncellemesi sirasinda ISR'nin       */
/* aramasini onler — veri tutarsizligi (race condition) engellenir        */

static uint8_t g_irq_disabled = 0U; /* PC simulasyonu icin               */

static void critical_enter(void)
{
    /* STM32 gercek kod: __disable_irq();                                 */
    g_irq_disabled = 1U; /* simulasyon — kesme devre disi                 */
    printf("  [IRQ disable]   : critical section girildi\n");
}

static void critical_exit(void)
{
    /* STM32 gercek kod: __enable_irq();                                  */
    g_irq_disabled = 0U; /* simulasyon — kesme yeniden aktif              */
    printf("  [IRQ enable]    : critical section cikti\n");
}

/* FLAG YONETIMI */

static void isr_flag_yonetimi(void)
{
    /* Flag tabanli ISR haberlesme — en yaygin gomulu sistem paterni      */
    /* ISR sadece flag set eder, gercek islem main() loop'unda yapilir   */
    /* ISR'yi kisa tutmak kritik — uzun ISR diger kesmeleri geciktirir   */

    /* ISR'yi simule et — gercekte donanim tetikler */
    USART1_IRQHandler_sim(0xAAU);
    USART1_IRQHandler_sim(0xBBU);

    /* main() loop'unda flag kontrol — polling                            */
    if (g_uart_rx_flag == 1U) /* veri hazir mi?                           */
    {
        g_uart_rx_flag = 0U; /* flag'i temizle — once temizle, sonra isle */
        /* flag onceden temizlenmezse islem sirasinda gelen yeni veri    */
        /* kaybolabilir — once temizle paterni kritik                    */
        printf("UART flag         : veri alindi, isleniyor\n");
    }
    else
    {
        printf("UART flag         : veri yok\n");
    }

    /* Hata flag kontrolu */
    if (g_hata_flag == 1U)
    {
        g_hata_flag = 0U;
        printf("Hata flag         : tampon dolulugu — veri kaybi\n");
    }
    else
    {
        /* hata yok — devam */
    }
}

/* ISR RING BUFFER KULLANIMI */

static void isr_ring_buffer(void)
{
    uint8_t okunan = 0U;
    uint8_t i;

    /* ISR'yi simule et — 5 byte gonder */
    for (i = 0U; i < 5U; i++)
    {
        USART1_IRQHandler_sim((uint8_t)(0x10U + i)); /* 0x10, 0x11, ... 0x14 */
    }

    printf("Ring doluluk      : %u\n", (uint32_t)g_uart_buf.dolu); /* 7 — onceki 2 + 5 */

    /* main() loop'unda ring buffer'dan oku                               */
    /* ISR yazarken, main() okur — volatile indeksler yarisi onler        */
    printf("Ring oku          : ");
    while (g_uart_buf.dolu > 0U) /* veri kaldigi surece oku              */
    {
        /* critical section — dolu azaltma atomic olmayabilir             */
        critical_enter();
        okunan             = g_uart_buf.buf[g_uart_buf.bas];
        g_uart_buf.bas     = (uint8_t)((g_uart_buf.bas + 1U) & (RING_MAX - 1U));
        g_uart_buf.dolu--;
        critical_exit();

        printf("0x%02X ", (uint32_t)okunan);
    }
    printf("\n");
}

/* CRITICAL SECTION */

static void isr_critical_section(void)
{
    /* 32-bit degisken guncellemesi — Cortex-M'de atomik olmayabilir      */
    /* ISR guncelleme sirasinda araya girerse yanlis deger okunabilir     */

    uint32_t yerel_sayac;

    /* Yanlis — critical section olmadan okuma                            */
    /* ISR g_systick_ms'i degistirirken okursak yanlis deger alabiliriz  */
    /* yerel_sayac = g_systick_ms; — TEHLIKELI                           */

    /* Dogru — critical section icinde oku                                */
    critical_enter();
    yerel_sayac = g_systick_ms; /* ISR araya giremez — guvenli okuma      */
    critical_exit();

    printf("Systick ms        : %u\n", (uint32_t)yerel_sayac);

    /* Timeout ornegi — ISR'nin periyodik flag setini bekle               */
    {
        uint32_t baslangic = g_systick_ms;
        uint32_t gecen;

        /* SysTick ISR'yi simule et — 50 kez cagir = 50ms                */
        uint8_t j;
        for (j = 0U; j < 50U; j++)
        {
            SysTick_Handler_sim();
        }

        gecen = g_systick_ms - baslangic; /* gecen sure hesapla           */
        printf("Gecen sure        : %u ms\n", (uint32_t)gecen); /* 50 */
    }
}

/* ZAMANLAYICI */

static void isr_zamanlayici(void)
{
    /* Non-blocking gecikme — HAL_Delay() bloklayan, bu pattern bloklayan */
    /* HAL_Delay() while loop'unda bekler, bu surede baska is yapamaz    */
    /* Asagidaki pattern CPU'yu mesgul etmeden zaman kontrolu saglar     */

    uint32_t son_tick   = g_systick_ms;
    uint32_t periyot_ms = 10U; /* 10ms periyot                            */
    uint8_t  i;

    printf("Non-blocking delay ornegi:\n");

    for (i = 0U; i < 3U; i++) /* 3 periyot goster                        */
    {
        /* SysTick'i simule et — periyot_ms kadar ilerlet                 */
        uint8_t j;
        for (j = 0U; j < (uint8_t)periyot_ms; j++)
        {
            SysTick_Handler_sim();
        }

        if ((g_systick_ms - son_tick) >= periyot_ms) /* periyot doldu mu? */
        {
            son_tick = g_systick_ms; /* bir sonraki periyot icin guncelle */
            printf("  Periyot %u       : tick=%u\n",
                   (uint32_t)(i + 1U), (uint32_t)g_systick_ms);

            /* TIM ISR'yi simule et */
            TIM2_IRQHandler_sim();
        }
        else
        {
            /* periyot dolmadi — diger islemlere devam et                 */
        }
    }

    printf("TIM kesme sayaci  : %u\n", (uint32_t)g_tim_sayac); /* 3 */
}

/* YAYGIN HATALAR */

static void yaygin_hatalar(void)
{
    /* 1) ISR'de uzun islem — diger kesmeleri geciktirir                  */
    {
        /* void USART1_IRQHandler(void) {                                 */
        /*     HAL_Delay(10);  — YANLIS: ISR'de bloklama kesinlikle yasak */
        /*     printf(...);    — YANLIS: ISR'de agir islem yapma          */
        /* }                                                               */
        /* Dogru: sadece flag set et veya ring buffer'a yaz, cik          */
        printf("ISR uzun islem    : flag set et, main()'de isle\n");
    }

    /* 2) volatile olmayan paylasilan degisken — optimize edilir          */
    {
        /* uint8_t flag = 0;                                              */
        /* while (flag == 0) { } — derleyici flag'i register'da tutar    */
        /* ISR flag=1 yapsa bile while'dan cikilamaz                      */
        /* Cozum: volatile uint8_t flag = 0U;                            */
        printf("volatile eksik    : ISR degiskenleri volatile olmali\n");
    }

    /* 3) Flag temizleme sirasi — veri kaybi riski                        */
    {
        /* Yanlis sira:                                                   */
        /* isle();          — once isle                                   */
        /* flag = 0U;       — sonra temizle: islem sirasinda gelen        */
        /*                    yeni kesme kaybolabilir                     */
        /*                                                                */
        /* Dogru sira:                                                    */
        /* flag = 0U;       — once temizle                                */
        /* isle();          — sonra isle: islem sirasinda gelen yeni      */
        /*                    kesme bir sonraki dongu iterasyonunda islenir */
        printf("Flag siralama     : once temizle, sonra isle\n");
    }

    /* 4) Critical section unutmak — race condition                       */
    {
        /* uint32_t t = g_systick_ms; — 32-bit okuma atomik olmayabilir   */
        /* ISR okuma ortasinda araya girerse yanlis deger alinir          */
        /* Cozum: critical section icinde oku                             */
        printf("Race condition    : 32-bit okuma critical section ile\n");
    }
}

/* MAIN */

int main(void)
{
    printf("/* ISR FLAG YONETIMI */\n");
    isr_flag_yonetimi();

    printf("\n/* ISR RING BUFFER */\n");
    isr_ring_buffer();

    printf("\n/* CRITICAL SECTION */\n");
    isr_critical_section();

    printf("\n/* ZAMANLAYICI */\n");
    isr_zamanlayici();

    printf("\n/* YAYGIN HATALAR */\n");
    yaygin_hatalar();

    return 0;
}