#include <stdint.h>
#include <stdio.h>
#include <stddef.h>  /* NULL icin */

/*
 * Bellek Yonetimi
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule  8.7  — sadece bir dosyadan erisilen nesne static olmali
 *   Rule 11.5  — void pointer baska tipe cast edilmemeli
 *   Rule 14.4  — if kosulu esansiyel boolean tipinde olmali
 *   Rule 21.3  — malloc/free kullanimi yasak
 *   Rule 21.4  — setjmp.h kullanimi yasak
 */

/* PROTOTIP BILDIRIMLERI */

static void stack_bellek(void);
static void static_bellek(void);
static void global_bellek(void);
static void statik_allocasyon(void);
static void bellek_hizalama(void);
static void yaygin_hatalar(void);

/* SABIT TANIMLAR */

#define RING_BUF_BOY   (8U)   /* ring buffer boyutu — 2'nin kuvveti olmali */
#define HAVUZ_BOY      (16U)  /* statik bellek havuzu boyutu                */
#define BLOK_BOY       (4U)   /* havuzdaki her blogun boyutu                */
#define BLOK_ADET      (HAVUZ_BOY / BLOK_BOY) /* toplam blok adedi          */

/* GLOBAL BELLEK */

/* Modul genelinde kullanilan statik degiskenler — .data veya .bss segmenti */
static uint8_t  g_rx_buf[64U]  = {0U}; /* UART alma tamponu — .data        */
static uint8_t  g_tx_buf[64U]  = {0U}; /* UART gonderme tamponu            */
static uint8_t  g_rx_head      = 0U;   /* ring buffer bas indeksi          */
static uint8_t  g_rx_tail      = 0U;   /* ring buffer kuyruk indeksi       */
static uint32_t g_hata_sayaci  = 0U;   /* hata sayaci — ISR ile paylasilan */

/* STATIK BELLEK HAVUZU */

/* Gomulede malloc yasak — sabit boyutlu havuz ile dinamik gibi davranilir */
typedef struct {
    uint8_t  veri[BLOK_BOY]; /* blok verisi      */
    uint8_t  kullanimda;     /* 1=dolu, 0=bos    */
} BellekBlok_t;

static BellekBlok_t g_havuz[BLOK_ADET]; /* statik bellek havuzu */

/* RING BUFFER YAPISI */

typedef struct {
    uint8_t  buf[RING_BUF_BOY]; /* tampon alani              */
    uint8_t  bas;               /* okuma indeksi             */
    uint8_t  son;               /* yazma indeksi             */
    uint8_t  dolu;              /* eleman sayisi             */
} RingBuf_t;

static RingBuf_t g_ring = {{0U}, 0U, 0U, 0U};

/* RING BUFFER FONKSİYONLARI */

static uint8_t ring_yaz(RingBuf_t * const rb, uint8_t veri)
{
    uint8_t ret;

    if (rb->dolu >= RING_BUF_BOY) /* dolu mu kontrol */
    {
        ret = 0U; /* basarisiz — yer yok */
    }
    else
    {
        rb->buf[rb->son] = veri;
        rb->son          = (uint8_t)((rb->son + 1U) & (RING_BUF_BOY - 1U)); /* modulo — 2'nin kuvveti oldugu icin & kullanildi */
        rb->dolu++;
        ret = 1U; /* basarili */
    }

    return ret;
}

static uint8_t ring_oku(RingBuf_t * const rb, uint8_t * const veri_out)
{
    uint8_t ret;

    if (rb->dolu == 0U) /* bos mu kontrol */
    {
        ret = 0U; /* basarisiz — veri yok */
    }
    else
    {
        *veri_out = rb->buf[rb->bas];
        rb->bas   = (uint8_t)((rb->bas + 1U) & (RING_BUF_BOY - 1U));
        rb->dolu--;
        ret = 1U; /* basarili */
    }

    return ret;
}

/* BELLEK HAVUZU FONKSİYONLARI */

static BellekBlok_t *havuz_al(void)
{
    uint8_t        i;
    BellekBlok_t  *ret = NULL;

    for (i = 0U; i < BLOK_ADET; i++)
    {
        if (g_havuz[i].kullanimda == 0U) /* bos blok ara */
        {
            g_havuz[i].kullanimda = 1U;
            ret = &g_havuz[i];
            break; /* ilk bos blok bulundu */
        }
        else
        {
            /* devam et */
        }
    }

    return ret; /* NULL donerse havuz dolu */
}

static void havuz_birak(BellekBlok_t * const blok)
{
    if (blok != NULL) /* NULL kontrolu zorunlu */
    {
        blok->kullanimda = 0U;
        /* veriyi sifirla — eski veri kalmasi onlendi */
        uint8_t i;
        for (i = 0U; i < BLOK_BOY; i++)
        {
            blok->veri[i] = 0U;
        }
    }
    else
    {
        /* gecersiz pointer — islem yapma */
    }
}

/* STACK BELLEK */

static void stack_bellek(void)
{
    /* Stack — fonksiyon cagrisinda otomatik olusur, cikista yok olur */
    uint8_t  yerel_a  = 10U;  /* stack'te — fonksiyon bitince gecersiz     */
    uint8_t  yerel_b  = 20U;
    uint8_t  dizi[8U] = {0U}; /* kucuk diziler stack'te — buyuk diziler static olmali */
    uint8_t  i;

    for (i = 0U; i < 8U; i++)
    {
        dizi[i] = (uint8_t)(i * yerel_a); /* stack degiskeni kullanimi */
    }

    printf("Stack degisken    : a=%u b=%u\n", (uint32_t)yerel_a, (uint32_t)yerel_b);
    printf("Stack dizi[3]     : %u\n", (uint32_t)dizi[3U]); /* 30 */

    /* STM32 varsayilan stack boyutu 512 byte — buyuk yerel dizi tehlikeli */
    /* uint8_t buyuk_dizi[512U]; — TEHLIKE: stack overflow riski */
}

/* STATIC BELLEK */

static void static_bellek(void)
{
    /* static yerel — fonksiyon cikinca yok olmaz, deger korunur */
    static uint32_t cagri_sayisi = 0U; /* ilk cagri 0, her cagride artar */

    cagri_sayisi++;
    printf("Cagri sayisi      : %u\n", (uint32_t)cagri_sayisi);

    /* static dizi — stack yerine .bss segmentinde */
    static uint8_t tampon[32U]; /* stack'i doldurmaz */
    tampon[0U] = (uint8_t)(cagri_sayisi & 0xFFU);
    printf("Static tampon[0]  : %u\n", (uint32_t)tampon[0U]);
}

/* GLOBAL BELLEK */

static void global_bellek(void)
{
    /* Global static degiskenleri kullan */
    g_rx_buf[0U] = 0xAAU;
    g_rx_buf[1U] = 0xBBU;
    g_tx_buf[0U] = g_rx_buf[0U]; /* rx'ten tx'e kopyala */

    g_rx_head = 0U;
    g_rx_tail = 2U;
    g_hata_sayaci++;

    printf("RX buf[0]         : 0x%02X\n", (uint32_t)g_rx_buf[0U]);
    printf("TX buf[0]         : 0x%02X\n", (uint32_t)g_tx_buf[0U]);
    printf("Hata sayaci       : %u\n",     (uint32_t)g_hata_sayaci);
    printf("RX bas/son        : %u/%u\n",  (uint32_t)g_rx_head, (uint32_t)g_rx_tail);
}

/* STATIK ALLOCASYON */

static void statik_allocasyon(void)
{
    /* Ring buffer kullanimi */
    uint8_t yaz_ret;
    uint8_t oku_ret;
    uint8_t okunan = 0U;
    uint8_t i;

    printf("Ring buffer yaz   : ");
    for (i = 0U; i < 5U; i++)
    {
        yaz_ret = ring_yaz(&g_ring, (uint8_t)(i + 1U));
        printf("%u(%s) ", (uint32_t)(i + 1U), (yaz_ret == 1U) ? "ok" : "err");
    }
    printf("\n");

    printf("Ring buffer oku   : ");
    for (i = 0U; i < 3U; i++)
    {
        oku_ret = ring_oku(&g_ring, &okunan);
        if (oku_ret == 1U)
        {
            printf("%u ", (uint32_t)okunan);
        }
        else
        {
            printf("err ");
        }
    }
    printf("\n");

    printf("Ring dolu         : %u\n", (uint32_t)g_ring.dolu); /* 2 */

    /* Bellek havuzu kullanimi */
    BellekBlok_t *blok1 = havuz_al();
    BellekBlok_t *blok2 = havuz_al();

    if (blok1 != NULL) /* NULL kontrolu zorunlu */
    {
        blok1->veri[0U] = 0xAAU;
        blok1->veri[1U] = 0xBBU;
        printf("Havuz blok1[0]    : 0x%02X\n", (uint32_t)blok1->veri[0U]);
    }
    else
    {
        printf("Havuz dolu        : blok alinamadi\n");
    }

    if (blok2 != NULL)
    {
        blok2->veri[0U] = 0xCCU;
        printf("Havuz blok2[0]    : 0x%02X\n", (uint32_t)blok2->veri[0U]);
    }
    else
    {
        printf("Havuz dolu        : blok alinamadi\n");
    }

    havuz_birak(blok1); /* blogu serbest birak */
    havuz_birak(blok2);
    printf("Havuz serbest     : bloklar birakildi\n");
}

/* BELLEK HIZALAMA */

static void bellek_hizalama(void)
{
    /* STM32'de yanlis hizalama HardFault olusturur */
    uint32_t deger32 = 0x12345678U;
    uint8_t  dizi[8U] = {0U};
    uint8_t  i;

    /* uint32_t veriyi byte dizisine yaz — Little Endian (STM32) */
    dizi[0U] = (uint8_t)(deger32 & 0xFFU);          /* LSB */
    dizi[1U] = (uint8_t)((deger32 >> 8U)  & 0xFFU);
    dizi[2U] = (uint8_t)((deger32 >> 16U) & 0xFFU);
    dizi[3U] = (uint8_t)((deger32 >> 24U) & 0xFFU); /* MSB */

    printf("Little Endian     : ");
    for (i = 0U; i < 4U; i++)
    {
        printf("0x%02X ", (uint32_t)dizi[i]);
    }
    printf("\n");

    /* Geri al — byte dizisinden uint32_t olustur */
    uint32_t geri = ((uint32_t)dizi[0U])         |
                    ((uint32_t)dizi[1U] << 8U)   |
                    ((uint32_t)dizi[2U] << 16U)  |
                    ((uint32_t)dizi[3U] << 24U);

    printf("Geri cevirme      : 0x%08X\n", (uint32_t)geri); /* 0x12345678 */
}

/* YAYGIN HATALAR */

static void yaygin_hatalar(void)
{
    /* 1) Stack overflow — buyuk yerel dizi */
    {
        /* uint8_t buyuk[1024U]; — TEHLIKE: STM32 default stack 512 byte */
        static uint8_t buyuk[1024U]; /* cozum: static yap — .bss'e gider */
        buyuk[0U] = 0xAAU;
        printf("Stack overflow    : onlendi (static kullanildi)\n");
        (void)buyuk;
    }

    /* 2) Dangling pointer — scope biten veriye erisim */
    {
        uint8_t *ptr = NULL;
        {
            uint8_t yerel = 42U;
            ptr = &yerel;
            printf("Yerel (icerde)    : %u\n", (uint32_t)*ptr); /* gecerli */
        }
        ptr = NULL; /* scope bitti — hemen NULL'a cek */
        printf("Dangling          : onlendi (NULL yapildi)\n");
    }

    /* 3) Buffer overflow — sinir asimi */
    {
        uint8_t buf[4U] = {0U};
        uint8_t i;
        /* for(i=0; i<=4; i++) buf[i]=i; — YANLIS: buf[4] gecersiz */
        for (i = 0U; i < 4U; i++) /* dogru: < 4U */
        {
            buf[i] = i;
        }
        printf("Buffer overflow   : onlendi\n");
        (void)buf;
    }

    /* 4) Malloc kullanimi — gomulede yasak */
    /* void *ptr = malloc(10); — MISRA Rule 21.3 ihlali */
    /* cozum: statik havuz veya static dizi kullan        */
    printf("Malloc            : yasak — statik havuz kullanildi\n");
}

/* MAIN */

int main(void)
{
    printf("/* STACK BELLEK */\n");
    stack_bellek();

    printf("\n/* STATIC BELLEK */\n");
    static_bellek();
    static_bellek(); /* iki kez cagir — sayacin arttigi gorulur */

    printf("\n/* GLOBAL BELLEK */\n");
    global_bellek();

    printf("\n/* STATIK ALLOCASYON */\n");
    statik_allocasyon();

    printf("\n/* BELLEK HIZALAMA */\n");
    bellek_hizalama();

    printf("\n/* YAYGIN HATALAR */\n");
    yaygin_hatalar();

    return 0;
}