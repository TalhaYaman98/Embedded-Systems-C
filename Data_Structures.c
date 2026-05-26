#include <stdint.h>
#include <stdio.h>
#include <stddef.h> /* NULL icin */

/*
 * Veri Yapilari
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule  8.7  — sadece bir dosyadan erisilen nesne static olmali
 *   Rule 11.5  — void pointer baska tipe cast edilmemeli
 *   Rule 14.4  — if kosulu esansiyel boolean tipinde olmali
 *   Rule 15.5  — fonksiyonun tek return noktasi olmali
 *   Rule 18.1  — pointer gecerli aralik icinde olmali
 *   Rule 21.3  — malloc/free kullanimi yasak
 */

/* PROTOTIP BILDIRIMLERI */

static void linked_list(void);
static void queue(void);
static void ring_buffer(void);
static void yaygin_hatalar(void);

/* SABIT TANIMLAR */

#define LIST_MAX     (8U)  /* bagli liste maksimum eleman sayisi — statik havuz */
#define QUEUE_MAX    (8U)  /* kuyruk maksimum eleman sayisi                     */
#define RING_MAX     (8U)  /* ring buffer boyutu — 2'nin kuvveti olmali         */

/* LINKED LIST */

/* Bagli liste dugum yapisi — her dugum kendinden sonrakini gosterir       */
/* Gomulede dinamik bellek yasak — statik havuzdan dugum alinir            */
typedef struct ListNode {
    uint8_t          veri;    /* dugumun tasidigi veri                     */
    struct ListNode *sonraki; /* bir sonraki dugume pointer, son dugumde NULL */
} ListNode_t;

/* Statik dugum havuzu — malloc yerine sabit boyutlu dizi kullanilir      */
static ListNode_t g_list_havuz[LIST_MAX];
static uint8_t    g_list_havuz_idx = 0U; /* bir sonraki bos havuz indeksi */

/* Havuzdan dugum al — malloc yerine gecen statik allocasyon              */
static ListNode_t *list_dugum_al(uint8_t veri)
{
    ListNode_t *ret = NULL;

    if (g_list_havuz_idx < LIST_MAX) /* havuz doluluk kontrolu            */
    {
        g_list_havuz[g_list_havuz_idx].veri     = veri;
        g_list_havuz[g_list_havuz_idx].sonraki  = NULL; /* yeni dugum listede son */
        ret = &g_list_havuz[g_list_havuz_idx];
        g_list_havuz_idx++;
    }
    else
    {
        /* havuz dolu — NULL doner, cagiran kontrol etmeli                */
    }

    return ret;
}

/* Listenin sonuna dugum ekle — bos liste icin basi gunceller             */
static void list_ekle(ListNode_t **bas, uint8_t veri)
{
    ListNode_t *yeni = list_dugum_al(veri); /* havuzdan dugum al          */

    if (yeni == NULL) /* havuz dolu — eklenemedi                          */
    {
        return;
    }
    else if (*bas == NULL) /* liste bos — yeni dugum bas olur              */
    {
        *bas = yeni;
    }
    else
    {
        /* listenin sonunu bul — son dugumun sonraki pointer'i NULL'dir   */
        ListNode_t *gez = *bas;
        while (gez->sonraki != NULL)
        {
            gez = gez->sonraki; /* bir sonraki dugume gec                 */
        }
        gez->sonraki = yeni; /* son dugumun arkasina ekle                 */
    }
}

/* Listeyi bastir — bas'tan sona kadar tum dugum verilerini yazar         */
static void list_yazdir(const ListNode_t *bas)
{
    const ListNode_t *gez = bas; /* gezici pointer — bas degistirilmez    */

    printf("Liste             : ");
    while (gez != NULL) /* NULL'a ulasinca liste bitti                    */
    {
        printf("%u ", (uint32_t)gez->veri);
        gez = gez->sonraki; /* bir sonraki dugume ilerle                  */
    }
    printf("\n");
}

/* Listede arama — veri bulunursa pointer, bulunamazsa NULL doner         */
static const ListNode_t *list_ara(const ListNode_t *bas, uint8_t aranan)
{
    const ListNode_t *gez = bas;
    const ListNode_t *ret = NULL;

    while (gez != NULL)
    {
        if (gez->veri == aranan) /* eslesen dugum bulundu                 */
        {
            ret = gez;
            gez = NULL; /* donguden cik — break MISRA'da kisitli          */
        }
        else
        {
            gez = gez->sonraki; /* aramaya devam et                       */
        }
    }

    return ret; /* tek return noktasi                                     */
}

static void linked_list(void)
{
    ListNode_t *bas = NULL; /* bos liste — bas pointer NULL               */

    /* Listeye eleman ekle */
    list_ekle(&bas, 10U);
    list_ekle(&bas, 20U);
    list_ekle(&bas, 30U);
    list_ekle(&bas, 40U);

    list_yazdir(bas); /* 10 20 30 40 */

    /* Listede arama */
    const ListNode_t *bulunan = list_ara(bas, 30U);
    if (bulunan != NULL) /* NULL kontrolu zorunlu — bulunamayabilir        */
    {
        printf("Bulunan           : %u\n", (uint32_t)bulunan->veri); /* 30 */
    }
    else
    {
        printf("Bulunan           : yok\n");
    }
}

/* QUEUE — FIFO (First In First Out) */

/* Kuyruk yapisi — ilk giren ilk cikar, UART/mesaj islemede yaygin        */
typedef struct {
    uint8_t buf[QUEUE_MAX]; /* kuyruk tamponu — statik, sabit boyut        */
    uint8_t bas;            /* okuma indeksi — en eski eleman buradan okunur */
    uint8_t son;            /* yazma indeksi — yeni eleman buraya yazilir  */
    uint8_t dolu;           /* mevcut eleman sayisi — tas/bos kontrolu icin */
} Queue_t;

/* Kuyruğu sifirla — tum alanlari baslangic degerine getir                */
static void queue_init(Queue_t * const q)
{
    uint8_t i;

    q->bas  = 0U;
    q->son  = 0U;
    q->dolu = 0U;

    for (i = 0U; i < QUEUE_MAX; i++) /* tamponu temizle                   */
    {
        q->buf[i] = 0U;
    }
}

/* Kuyruğa eleman ekle — yer yoksa basarisiz doner                        */
static uint8_t queue_yaz(Queue_t * const q, uint8_t veri)
{
    uint8_t ret;

    if (q->dolu >= QUEUE_MAX) /* kuyruk dolu — overflow onlendi            */
    {
        ret = 0U; /* basarisiz */
    }
    else
    {
        q->buf[q->son] = veri;
        q->son         = (uint8_t)((q->son + 1U) % QUEUE_MAX); /* dairesel indeks */
        q->dolu++;
        ret = 1U; /* basarili */
    }

    return ret;
}

/* Kuyruktan eleman oku — FIFO sirasi korunur, bos ise basarisiz doner    */
static uint8_t queue_oku(Queue_t * const q, uint8_t * const veri_out)
{
    uint8_t ret;

    if (q->dolu == 0U) /* kuyruk bos — underflow onlendi                  */
    {
        ret = 0U; /* basarisiz */
    }
    else
    {
        *veri_out = q->buf[q->bas];
        q->bas    = (uint8_t)((q->bas + 1U) % QUEUE_MAX); /* dairesel indeks */
        q->dolu--;
        ret = 1U; /* basarili */
    }

    return ret;
}

static void queue(void)
{
    Queue_t q;
    uint8_t okunan = 0U;
    uint8_t ret;

    queue_init(&q); /* kuyruğu sifirla — baslatilmamis kullanim onlendi   */

    /* Kuyruğa eleman ekle — FIFO sirasi: ilk giren ilk cikar             */
    (void)queue_yaz(&q, 0xAAU);
    (void)queue_yaz(&q, 0xBBU);
    (void)queue_yaz(&q, 0xCCU);
    printf("Kuyruk doluluk    : %u\n", (uint32_t)q.dolu); /* 3 */

    /* Kuyruktan oku — ekleme sirasinda cikmali: AA, BB, CC               */
    ret = queue_oku(&q, &okunan);
    if (ret == 1U)
    {
        printf("Kuyruk okunan     : 0x%02X\n", (uint32_t)okunan); /* 0xAA */
    }
    else
    {
        printf("Kuyruk bos\n");
    }

    ret = queue_oku(&q, &okunan);
    if (ret == 1U)
    {
        printf("Kuyruk okunan     : 0x%02X\n", (uint32_t)okunan); /* 0xBB */
    }
    else
    {
        printf("Kuyruk bos\n");
    }

    printf("Kuyruk doluluk    : %u\n", (uint32_t)q.dolu); /* 1 */
}

/* RING BUFFER */

/* Ring buffer yapisi — sabit bellek ile surekli veri akisi saglar        */
/* UART RX gibi kesme tabanli veri aliminda en yaygin kullanim            */
/* Boyut 2'nin kuvveti olmali — modulo yerine AND kullanilabilir          */
typedef struct {
    uint8_t  buf[RING_MAX]; /* tampon — dairesel kullanilir                */
    uint8_t  bas;           /* okuma indeksi                               */
    uint8_t  son;           /* yazma indeksi                               */
    uint8_t  dolu;          /* mevcut byte sayisi                          */
} RingBuf_t;

static void ring_init(RingBuf_t * const rb)
{
    uint8_t i;

    rb->bas  = 0U;
    rb->son  = 0U;
    rb->dolu = 0U;

    for (i = 0U; i < RING_MAX; i++)
    {
        rb->buf[i] = 0U; /* tamponu sifirla — eski veri kalmasi onlendi   */
    }
}

/* Ring buffer'a yaz — son indeksi ilerler, tasar basa doner              */
static uint8_t ring_yaz(RingBuf_t * const rb, uint8_t veri)
{
    uint8_t ret;

    if (rb->dolu >= RING_MAX) /* tampon dolu — veri kaybi                 */
    {
        ret = 0U;
    }
    else
    {
        rb->buf[rb->son] = veri;
        /* & ile modulo — RING_MAX 2'nin kuvveti oldugu icin gecerli      */
        rb->son  = (uint8_t)((rb->son + 1U) & (RING_MAX - 1U));
        rb->dolu++;
        ret = 1U;
    }

    return ret;
}

/* Ring buffer'dan oku — bas indeksi ilerler, tasar basa doner            */
static uint8_t ring_oku(RingBuf_t * const rb, uint8_t * const veri_out)
{
    uint8_t ret;

    if (rb->dolu == 0U) /* tampon bos — okunacak veri yok                 */
    {
        ret = 0U;
    }
    else
    {
        *veri_out = rb->buf[rb->bas];
        rb->bas   = (uint8_t)((rb->bas + 1U) & (RING_MAX - 1U));
        rb->dolu--;
        ret = 1U;
    }

    return ret;
}

static void ring_buffer(void)
{
    RingBuf_t rb;
    uint8_t   okunan = 0U;
    uint8_t   i;

    ring_init(&rb);

    /* 6 eleman yaz — RING_MAX=8, yer var                                 */
    printf("Ring yaz          : ");
    for (i = 1U; i <= 6U; i++)
    {
        (void)ring_yaz(&rb, (uint8_t)(i * 10U));
        printf("%u ", (uint32_t)(i * 10U));
    }
    printf("\n");
    printf("Ring doluluk      : %u\n", (uint32_t)rb.dolu); /* 6 */

    /* 3 eleman oku — FIFO sirasi korunur                                 */
    printf("Ring oku          : ");
    for (i = 0U; i < 3U; i++)
    {
        if (ring_oku(&rb, &okunan) == 1U)
        {
            printf("%u ", (uint32_t)okunan); /* 10 20 30 */
        }
        else
        {
            printf("bos ");
        }
    }
    printf("\n");
    printf("Ring doluluk      : %u\n", (uint32_t)rb.dolu); /* 3 */

    /* Yeni eleman yaz — dairesel yapi, bos yerlere devam eder            */
    (void)ring_yaz(&rb, 0xAAU);
    (void)ring_yaz(&rb, 0xBBU);
    printf("Ring doluluk      : %u\n", (uint32_t)rb.dolu); /* 5 */
}

/* YAYGIN HATALAR */

static void yaygin_hatalar(void)
{
    /* 1) NULL pointer kontrolsuz erisim — cokme riski                    */
    {
        ListNode_t *ptr = NULL;
        /* ptr->veri = 5U; — YANLIS: NULL dereference, HardFault          */
        if (ptr != NULL) /* NULL kontrolu zorunlu, sonra eris              */
        {
            ptr->veri = 5U;
        }
        else
        {
            printf("NULL kontrol      : pointer bos, erisim atlandi\n");
        }
    }

    /* 2) Ring buffer boyutunun 2'nin kuvveti olmamasi                    */
    {
        /* #define RING_MAX (7U) — YANLIS: & ile modulo calismaz          */
        /* (idx + 1) & (7-1) = (idx+1) & 6 — yanlis sarma                */
        /* cozum: RING_MAX daima 2,4,8,16,32... olmali                    */
        printf("Ring boyutu       : 2'nin kuvveti olmali (8,16,32...)\n");
    }

    /* 3) Kuyruk doluyken yazma — sessiz veri kaybi                       */
    {
        Queue_t q;
        uint8_t ret;
        uint8_t i;

        queue_init(&q);

        /* QUEUE_MAX kadar doldur */
        for (i = 0U; i < QUEUE_MAX; i++)
        {
            (void)queue_yaz(&q, i);
        }

        /* bir fazla yazmayi dene — donus degeri kontrol edilmeli         */
        ret = queue_yaz(&q, 0xFFU);
        if (ret == 0U) /* donus degeri kontrol edilmezse veri kaybi farkedilmez */
        {
            printf("Kuyruk dolu       : yazma reddedildi, veri kaybi onlendi\n");
        }
        else
        {
            printf("Kuyruk dolu       : beklenmedik durum\n");
        }
    }

    /* 4) Statik havuz tukenmesi — liste dugumu alinamaz                  */
    {
        /* g_list_havuz_idx == LIST_MAX ise list_dugum_al NULL doner      */
        /* cagiran fonksiyon NULL kontrolu yapmazsa HardFault olusur      */
        /* cozum: havuz boyutunu uygulama ihtiyacina gore ayarla          */
        printf("Havuz tukenme     : NULL kontrolu zorunlu\n");
    }
}

/* MAIN */

int main(void)
{
    printf("/* LINKED LIST */\n");
    linked_list();

    printf("\n/* QUEUE */\n");
    queue();

    printf("\n/* RING BUFFER */\n");
    ring_buffer();

    printf("\n/* YAYGIN HATALAR */\n");
    yaygin_hatalar();

    return 0;
}