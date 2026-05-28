#include <stdint.h>
#include <stdio.h>

typedef float float32_t;

/*
 * Filtreler
 * MISRA-C:2012 Uygulamalari
 *
 * Kullanilan kurallar:
 *   Rule  7.2  — unsigned sabit literaller U suffix tasimali
 *   Rule  8.1  — tipler acikca belirtilmeli
 *   Rule  8.7  — sadece bir dosyadan erisilen nesne static olmali
 *   Rule 10.3  — atama hedef tip ile uyumlu olmali
 *   Rule 14.4  — if kosulu esansiyel boolean tipinde olmali
 *   Rule 15.5  — fonksiyonun tek return noktasi olmali
 *
 * Filtre secim rehberi:
 *
 *   Olcum tipi              Onerilен filtre
 *   ─────────────────────────────────────────────────────
 *   ADC / gerilim           EMA veya Moving Average
 *   Sicaklik                EMA (yavash sistem)
 *   Basinc                  IIR Alçak geciren
 *   IMU / ivme              Kalman veya Alpha-Beta
 *   Enkoder / hiz           Weighted Moving Average
 *   Buton / dijital         Debounce
 *   Gurultulu analog        Median + EMA kombine
 *   Ses / titresim          FIR / Butterworth
 *   Guc / akim              Moving Average
 *   ─────────────────────────────────────────────────────
 */

/* PROTOTIP BILDIRIMLERI */

static void moving_average_ornegi(void);
static void weighted_moving_average_ornegi(void);
static void ema_ornegi(void);
static void median_ornegi(void);
static void iir_ornegi(void);
static void fir_ornegi(void);
static void kalman_ornegi(void);
static void butterworth_ornegi(void);
static void alpha_beta_ornegi(void);
static void debounce_ornegi(void);

/* ================================================================== */
/* 1. MOVING AVERAGE (MA) — BASIT HAREKETLI ORTALAMA                  */
/* ================================================================== */

/*
 * Calisma prensibi:
 *   Son N ornegi tamponlar, hepsinin aritmetik ortalamasini alir.
 *   Yeni ornek gelince en eski ornek atilir — kayar pencere.
 *
 * Kullanim alanlari:
 *   ADC okuma, guc/akim olcumu, gerilim izleme
 *
 * Avantaj  : basit, anlasilir, sabit gecikme
 * Dezavantaj: N kadar bellek gerekir, yavash yanit
 */

#define MA_PENCERE  (8U)  /* pencere boyutu — 2'nin kuvveti olursa      */
                          /* bolme islemi shift ile yapilabilir          */

typedef struct {
    float32_t tampon[MA_PENCERE]; /* ornek tamponu                      */
    uint8_t   indeks;             /* bir sonraki yazma konumu            */
    uint8_t   dolu;               /* tampon doluluk sayaci               */
    float32_t toplam;             /* toplam — her adimda guncellenir     */
} MA_t;

/* MA baslat — tum alanlari sifirla                                     */
static void MA_Init(MA_t * const f)
{
    uint8_t i;
    f->indeks = 0U;
    f->dolu   = 0U;
    f->toplam = 0.0f;
    for (i = 0U; i < MA_PENCERE; i++)
    {
        f->tampon[i] = 0.0f;
    }
}

/* MA guncelle — yeni ornek ekle, filtrelenmis degeri dondur            */
static float32_t MA_Guncelle(MA_t * const f, float32_t yeni)
{
    float32_t ret;

    /* En eski ornegi toplamdan cikar */
    f->toplam -= f->tampon[f->indeks];

    /* Yeni ornegi tampona yaz ve toplama ekle */
    f->tampon[f->indeks] = yeni;
    f->toplam           += yeni;

    /* Dairesel indeks ilerlet */
    f->indeks = (uint8_t)((f->indeks + 1U) % MA_PENCERE);

    /* Doluluk sayacini guncelle */
    if (f->dolu < MA_PENCERE)
    {
        f->dolu++;
    }
    else
    {
        /* tampon dolu — eski deger uzerine yazildi */
    }

    /* Ortalama hesapla — sifira bolme koruması */
    ret = (f->dolu > 0U) ? (f->toplam / (float32_t)f->dolu) : 0.0f;

    return ret;
}

static void moving_average_ornegi(void)
{
    MA_t      f;
    float32_t giris[]  = {10.0f, 12.0f, 100.0f, 11.0f, 13.0f,
                          10.0f, 12.0f, 11.0f,  14.0f, 10.0f};
    uint8_t   adet     = (uint8_t)(sizeof(giris) / sizeof(giris[0U]));
    uint8_t   i;

    MA_Init(&f);

    printf("  Moving Average (pencere=%u):\n", (uint32_t)MA_PENCERE);
    printf("  %-6s  %-10s  %-10s\n", "Adim", "Giris", "Cikis");

    for (i = 0U; i < adet; i++)
    {
        float32_t cikis = MA_Guncelle(&f, giris[i]);
        printf("  %-6u  %-10.2f  %-10.2f\n",
               (uint32_t)i, (double)giris[i], (double)cikis);
    }
    /* 100.0f spike gorulur ama ortalamaya yayilir — spike baskilamada   */
    /* median filtre daha iyidir                                         */
}

/* ================================================================== */
/* 2. WEIGHTED MOVING AVERAGE (WMA) — AGIRLIKLI HAREKETLI ORTALAMA    */
/* ================================================================== */

/*
 * Calisma prensibi:
 *   Son N ornege farkli agirlik verir. Yeni ornekler daha agir tartilir.
 *   Agirliklar: N, N-1, N-2 ... 1 (toplam = N*(N+1)/2)
 *
 * Kullanim alanlari:
 *   Enkoder hiz olcumu, motor akim takibi
 *
 * Avantaj  : yeni veriye daha duyarli, MA'dan hizli yanit
 * Dezavantaj: MA'dan hesapli olarak daha agir
 */

#define WMA_PENCERE  (5U)

typedef struct {
    float32_t tampon[WMA_PENCERE];
    uint8_t   indeks;
    uint8_t   dolu;
} WMA_t;

static void WMA_Init(WMA_t * const f)
{
    uint8_t i;
    f->indeks = 0U;
    f->dolu   = 0U;
    for (i = 0U; i < WMA_PENCERE; i++)
    {
        f->tampon[i] = 0.0f;
    }
}

static float32_t WMA_Guncelle(WMA_t * const f, float32_t yeni)
{
    float32_t agirlikli_toplam = 0.0f;
    float32_t agirlik_toplam   = 0.0f;
    uint8_t   i;
    uint8_t   pos;
    float32_t agirlik;

    /* Yeni ornegi tampona yaz */
    f->tampon[f->indeks] = yeni;
    f->indeks = (uint8_t)((f->indeks + 1U) % WMA_PENCERE);

    if (f->dolu < WMA_PENCERE)
    {
        f->dolu++;
    }
    else
    {
        /* tampon dolu */
    }

    /* En yeni ornekten baslayarak agirlikli toplam hesapla             */
    for (i = 0U; i < f->dolu; i++)
    {
        /* En yeni ornek en yuksek agirliga sahip */
        pos     = (uint8_t)((f->indeks + WMA_PENCERE - 1U - i) % WMA_PENCERE);
        agirlik = (float32_t)(f->dolu - i); /* N, N-1, N-2 ... 1        */

        agirlikli_toplam += f->tampon[pos] * agirlik;
        agirlik_toplam   += agirlik;
    }

    return (agirlik_toplam > 0.0f) ?
           (agirlikli_toplam / agirlik_toplam) : 0.0f;
}

static void weighted_moving_average_ornegi(void)
{
    WMA_t     f;
    float32_t giris[] = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f,
                         60.0f, 70.0f, 80.0f};
    uint8_t   adet    = (uint8_t)(sizeof(giris) / sizeof(giris[0U]));
    uint8_t   i;

    WMA_Init(&f);

    printf("  Weighted Moving Average (pencere=%u):\n", (uint32_t)WMA_PENCERE);
    printf("  %-6s  %-10s  %-10s\n", "Adim", "Giris", "Cikis");

    for (i = 0U; i < adet; i++)
    {
        float32_t cikis = WMA_Guncelle(&f, giris[i]);
        printf("  %-6u  %-10.2f  %-10.2f\n",
               (uint32_t)i, (double)giris[i], (double)cikis);
    }
    /* WMA, MA'ya gore artan sinyali daha hizli takip eder              */
}

/* ================================================================== */
/* 3. EMA — USSEL HAREKETLI ORTALAMA                                  */
/* ================================================================== */

/*
 * Calisma prensibi:
 *   y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
 *   alpha: 0..1 arasi — buyuk alpha = hizli yanit, kucuk = yumusak
 *
 * Kullanim alanlari:
 *   Sicaklik, basinc, gerilim, hiz olcumu — en yaygin filtre
 *
 * Avantaj  : tek degisken, minimal bellek, kolay ayar
 * Dezavantaj: alpha secimi deneyim gerektirir
 *
 * Alpha hesabi: alpha = 1 - e^(-Ts/tau)
 *   Ts  = ornekleme periyodu
 *   tau = zaman sabiti (sisteme gore belirlenir)
 */

typedef struct {
    float32_t cikis;      /* onceki filtre cikisi                        */
    float32_t alpha;      /* yumusaklama katsayisi — 0.0..1.0            */
    uint8_t   baslatildi; /* ilk ornek icin ozel islem                   */
} EMA_t;

static void EMA_Init(EMA_t * const f, float32_t alpha)
{
    f->cikis      = 0.0f;
    f->baslatildi = 0U;

    /* Alpha aralik kontrolu */
    if (alpha < 0.0f)
    {
        f->alpha = 0.01f; /* minimum — cok yumusak                       */
    }
    else if (alpha > 1.0f)
    {
        f->alpha = 1.0f; /* maksimum — filtresiz gecis                   */
    }
    else
    {
        f->alpha = alpha;
    }
}

static float32_t EMA_Guncelle(EMA_t * const f, float32_t yeni)
{
    if (f->baslatildi == 0U)
    {
        /* Ilk ornekte cikis = giris — soguk baslama etkisi onlendi      */
        f->cikis      = yeni;
        f->baslatildi = 1U;
    }
    else
    {
        /* EMA formulu: y = alpha*x + (1-alpha)*y_onceki                 */
        f->cikis = (f->alpha * yeni) + ((1.0f - f->alpha) * f->cikis);
    }

    return f->cikis;
}

static void ema_ornegi(void)
{
    EMA_t     f_yavash; /* alpha=0.1 — yavash, yumusak                  */
    EMA_t     f_hizli;  /* alpha=0.5 — hizli, daha az yumusak           */

    float32_t giris[] = {20.0f, 21.0f, 100.0f, 22.0f, 20.0f,
                         19.0f, 21.0f,  20.0f, 22.0f, 21.0f};
    uint8_t   adet    = (uint8_t)(sizeof(giris) / sizeof(giris[0U]));
    uint8_t   i;

    EMA_Init(&f_yavash, 0.1f);  /* sicaklik gibi yavash sistem          */
    EMA_Init(&f_hizli,  0.5f);  /* akim gibi hizli degisen sistem       */

    printf("  EMA Filtre:\n");
    printf("  %-6s  %-10s  %-14s  %-14s\n",
           "Adim", "Giris", "a=0.1(yavash)", "a=0.5(hizli)");

    for (i = 0U; i < adet; i++)
    {
        float32_t c1 = EMA_Guncelle(&f_yavash, giris[i]);
        float32_t c2 = EMA_Guncelle(&f_hizli,  giris[i]);
        printf("  %-6u  %-10.2f  %-14.2f  %-14.2f\n",
               (uint32_t)i, (double)giris[i], (double)c1, (double)c2);
    }
    /* alpha=0.1: 100.0f spike neredeyse gorulmez                        */
    /* alpha=0.5: spike daha belirgin ama hizli toparlanir               */
}

/* ================================================================== */
/* 4. MEDIAN FILTER — MEDYAN FILTRE                                    */
/* ================================================================== */

/*
 * Calisma prensibi:
 *   Son N ornegi siralar, ortanca degeri dondurur.
 *   Anlık spike ve impuls gurultusunu tamamen bastirir.
 *
 * Kullanim alanlari:
 *   ADC spike temizleme, ultrasonik mesafe sensoru,
 *   dokunmatik panel gurultusu
 *
 * Avantaj  : spike'a karsi en iyi koruma, kenar korur
 * Dezavantaj: N buyudukce hesap maliyeti artar (siralama O(N^2))
 */

#define MED_PENCERE  (5U)  /* tek sayi olmali — ortanca icin            */

typedef struct {
    float32_t tampon[MED_PENCERE];
    uint8_t   indeks;
    uint8_t   dolu;
} Median_t;

static void Median_Init(Median_t * const f)
{
    uint8_t i;
    f->indeks = 0U;
    f->dolu   = 0U;
    for (i = 0U; i < MED_PENCERE; i++)
    {
        f->tampon[i] = 0.0f;
    }
}

/* Bubble sort ile siralama — kucuk N icin yeterli, MISRA uyumlu        */
static float32_t Median_Guncelle(Median_t * const f, float32_t yeni)
{
    float32_t siralı[MED_PENCERE];
    float32_t tmp;
    uint8_t   i;
    uint8_t   j;
    uint8_t   adet;

    /* Yeni ornegi tampona yaz */
    f->tampon[f->indeks] = yeni;
    f->indeks = (uint8_t)((f->indeks + 1U) % MED_PENCERE);

    if (f->dolu < MED_PENCERE)
    {
        f->dolu++;
    }
    else
    {
        /* tampon dolu */
    }

    adet = f->dolu;

    /* Tamponu gecici diziye kopyala — orijinal bozulmasin              */
    for (i = 0U; i < adet; i++)
    {
        siralı[i] = f->tampon[i];
    }

    /* Bubble sort — kucuk N icin O(N^2) kabul edilebilir               */
    for (i = 0U; i < (uint8_t)(adet - 1U); i++)
    {
        for (j = 0U; j < (uint8_t)(adet - 1U - i); j++)
        {
            if (siralı[j] > siralı[j + 1U])
            {
                tmp          = siralı[j];
                siralı[j]     = siralı[j + 1U];
                siralı[j + 1U] = tmp;
            }
            else
            {
                /* sıra dogru */
            }
        }
    }

    /* Ortanca degeri dondur */
    return siralı[adet / 2U];
}

static void median_ornegi(void)
{
    Median_t  f;
    float32_t giris[] = {20.0f, 21.0f, 500.0f, 22.0f, 20.0f,
                         19.0f, 300.0f, 21.0f,  20.0f, 22.0f};
    uint8_t   adet    = (uint8_t)(sizeof(giris) / sizeof(giris[0U]));
    uint8_t   i;

    Median_Init(&f);

    printf("  Median Filtre (pencere=%u):\n", (uint32_t)MED_PENCERE);
    printf("  %-6s  %-10s  %-10s\n", "Adim", "Giris", "Cikis");

    for (i = 0U; i < adet; i++)
    {
        float32_t cikis = Median_Guncelle(&f, giris[i]);
        printf("  %-6u  %-10.2f  %-10.2f\n",
               (uint32_t)i, (double)giris[i], (double)cikis);
    }
    /* 500.0f ve 300.0f spike'lari cikista gorulmez                      */
}

/* ================================================================== */
/* 5. IIR FILTER — SONSUZ DURTÜ YANITLI FILTRE                        */
/* ================================================================== */

/*
 * Calisma prensibi:
 *   y[n] = b0*x[n] + b1*x[n-1] - a1*y[n-1]
 *   Geri besleme var — cikis bir onceki cikisa baglidir.
 *   EMA, birinci dereceden IIR filtrenin ozel halidir.
 *
 * Kullanim alanlari:
 *   Basinc sensoru, ses isleme, titresim analizi
 *
 * Avantaj  : dusuk hesap maliyeti, esnek frekans yaniti
 * Dezavantaj: kararsiz hale gelebilir — katsayi secimi kritik
 *
 * Katsayı hesabi:
 *   fc = kesim frekansi (Hz)
 *   fs = ornekleme frekansi (Hz)
 *   wc = 2*pi*fc/fs
 *   Birinci dereceden alcak geciren:
 *   b0 = wc/(wc+2), b1 = wc/(wc+2), a1 = (wc-2)/(wc+2)
 */

typedef struct {
    float32_t b0;     /* giris katsayisi — x[n]                         */
    float32_t b1;     /* giris katsayisi — x[n-1]                       */
    float32_t a1;     /* geri besleme katsayisi — y[n-1]                */
    float32_t x_prev; /* bir onceki giris                                */
    float32_t y_prev; /* bir onceki cikis                                */
} IIR_t;

/* fc=10Hz, fs=1000Hz icin onceden hesaplanmis katsayilar               */
/* wc = 2*pi*10/1000 = 0.0628                                           */
/* b0 = b1 = 0.0628/(0.0628+2) = 0.0304                                */
/* a1 = (0.0628-2)/(0.0628+2) = -0.9391                                 */
static void IIR_Init(IIR_t * const f,
                      float32_t b0, float32_t b1, float32_t a1)
{
    f->b0     = b0;
    f->b1     = b1;
    f->a1     = a1;
    f->x_prev = 0.0f;
    f->y_prev = 0.0f;
}

static float32_t IIR_Guncelle(IIR_t * const f, float32_t x)
{
    /* y[n] = b0*x[n] + b1*x[n-1] - a1*y[n-1]                         */
    float32_t y = (f->b0 * x) + (f->b1 * f->x_prev) - (f->a1 * f->y_prev);

    f->x_prev = x;   /* x[n] → x[n-1] */
    f->y_prev = y;   /* y[n] → y[n-1] */

    return y;
}

static void iir_ornegi(void)
{
    IIR_t     f;
    /* fc=10Hz, fs=1000Hz alcak geciren katsayilari                     */
    float32_t giris[] = {0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
                         1.0f,  1.0f,  1.0f,  1.0f,  1.0f};
    uint8_t   adet    = (uint8_t)(sizeof(giris) / sizeof(giris[0U]));
    uint8_t   i;

    IIR_Init(&f, 0.0304f, 0.0304f, -0.9391f);

    printf("  IIR Alcak Geciren (fc=10Hz, fs=1000Hz):\n");
    printf("  %-6s  %-10s  %-10s\n", "Adim", "Giris", "Cikis");

    for (i = 0U; i < adet; i++)
    {
        float32_t cikis = IIR_Guncelle(&f, giris[i]);
        printf("  %-6u  %-10.4f  %-10.4f\n",
               (uint32_t)i, (double)giris[i], (double)cikis);
    }
    /* Basamak girisine yumusak yanit — fc uzerindeki bilesenler baskilanir */
}

/* ================================================================== */
/* 6. FIR FILTER — SONLU DURTÜ YANITLI FILTRE                         */
/* ================================================================== */

/*
 * Calisma prensibi:
 *   y[n] = sum(h[k] * x[n-k]) k=0..N-1
 *   Geri besleme yok — her zaman kararli.
 *   Katsayilar (h) filtre tasarimindan gelir.
 *
 * Kullanim alanlari:
 *   Ses isleme, titresim analizi, EEG/ECG biyomedikal
 *
 * Avantaj  : her zaman kararli, dogrusal faz (gecikme sabiti)
 * Dezavantaj: IIR'ye gore daha fazla katsayi gerekir
 *
 * Katsayı hesabi: windowed sinc yontemi veya MATLAB/Python firwin()
 */

#define FIR_TAPIN   (7U)  /* filtre katsayi sayisi (tap)                */

typedef struct {
    float32_t tampon[FIR_TAPIN]; /* giris gecmisi                       */
    uint8_t   indeks;
} FIR_t;

/* fc=0.1*fs alcak geciren pencereli sinc katsayilari (Hamming pencere) */
static const float32_t FIR_KATSAYILAR[FIR_TAPIN] = {
    -0.0089f,  0.0000f,  0.2867f,  0.4444f,
     0.2867f,  0.0000f, -0.0089f
};

static void FIR_Init(FIR_t * const f)
{
    uint8_t i;
    f->indeks = 0U;
    for (i = 0U; i < FIR_TAPIN; i++)
    {
        f->tampon[i] = 0.0f;
    }
}

static float32_t FIR_Guncelle(FIR_t * const f, float32_t yeni)
{
    float32_t cikis = 0.0f;
    uint8_t   i;
    uint8_t   pos;

    /* Yeni ornegi tampona yaz */
    f->tampon[f->indeks] = yeni;
    f->indeks = (uint8_t)((f->indeks + 1U) % FIR_TAPIN);

    /* Konvolüsyon — katsayilarla carpip topla                          */
    for (i = 0U; i < FIR_TAPIN; i++)
    {
        pos     = (uint8_t)((f->indeks + FIR_TAPIN - 1U - i) % FIR_TAPIN);
        cikis  += FIR_KATSAYILAR[i] * f->tampon[pos];
    }

    return cikis;
}

static void fir_ornegi(void)
{
    FIR_t     f;
    float32_t giris[] = {0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                         1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    uint8_t   adet    = (uint8_t)(sizeof(giris) / sizeof(giris[0U]));
    uint8_t   i;

    FIR_Init(&f);

    printf("  FIR Alcak Geciren (%u tap):\n", (uint32_t)FIR_TAPIN);
    printf("  %-6s  %-10s  %-10s\n", "Adim", "Giris", "Cikis");

    for (i = 0U; i < adet; i++)
    {
        float32_t cikis = FIR_Guncelle(&f, giris[i]);
        printf("  %-6u  %-10.4f  %-10.4f\n",
               (uint32_t)i, (double)giris[i], (double)cikis);
    }
}

/* ================================================================== */
/* 7. KALMAN FILTER — DURUM TAHMINI                                    */
/* ================================================================== */

/*
 * Calisma prensibi:
 *   Sistem modeli ve olcum gurultusunu birlikte kullanarak
 *   en iyi durum tahminini uretir.
 *   Iki adim: tahmin (predict) + guncelleme (update)
 *
 * Kullanim alanlari:
 *   IMU (ivmemetre + jiroskop), GPS konum, motor hiz kontrolu
 *
 * Avantaj  : gurultuyu istatistiksel olarak en iyiler
 * Dezavantaj: sistem modelinin bilinmesi gerekir, R ve Q ayari kritik
 *
 * Parametreler:
 *   Q: islem gurultusu kovaryans — sistem ne kadar degisken?
 *   R: olcum gurultusu kovaryans — sensor ne kadar gurultulu?
 *   Kucuk R/Q → olcume guvен, buyuk R/Q → modele guvен
 */

typedef struct {
    float32_t x;  /* durum tahmini — filtrelenmis deger                  */
    float32_t p;  /* hata kovaryans tahmini                              */
    float32_t q;  /* islem gurultusu kovaryans                           */
    float32_t r;  /* olcum gurultusu kovaryans                           */
    float32_t k;  /* Kalman kazanci                                      */
} Kalman_t;

static void Kalman_Init(Kalman_t * const f,
                         float32_t q, float32_t r, float32_t baslangic)
{
    f->x = baslangic; /* baslangic tahmini                               */
    f->p = 1.0f;      /* baslangic hata kovaryans                       */
    f->q = q;         /* islem gurultusu — ne kadar hizli degisiyor?    */
    f->r = r;         /* olcum gurultusu — sensor ne kadar gurultulu?   */
    f->k = 0.0f;
}

static float32_t Kalman_Guncelle(Kalman_t * const f, float32_t olcum)
{
    /* Tahmin adimi (Predict)                                            */
    /* p = p + q — hata kovaryans artar, belirsizlik buyur              */
    f->p = f->p + f->q;

    /* Guncelleme adimi (Update)                                         */
    /* k = p / (p + r) — Kalman kazanci hesapla                        */
    f->k = f->p / (f->p + f->r);

    /* x = x + k * (olcum - x) — tahmin guncelle                       */
    f->x = f->x + (f->k * (olcum - f->x));

    /* p = (1 - k) * p — hata kovaryans guncelle                       */
    f->p = (1.0f - f->k) * f->p;

    return f->x;
}

static void kalman_ornegi(void)
{
    Kalman_t  f;
    /* Gercek deger 25.0, gurultulu olcumler */
    float32_t giris[] = {24.5f, 25.8f, 23.9f, 26.2f, 24.8f,
                         25.3f, 24.1f, 25.9f, 25.2f, 24.7f};
    uint8_t   adet    = (uint8_t)(sizeof(giris) / sizeof(giris[0U]));
    uint8_t   i;

    /* q=0.01 (sistem az degisiyor), r=1.0 (sensor gurultulu)           */
    Kalman_Init(&f, 0.01f, 1.0f, 25.0f);

    printf("  Kalman Filtre (q=0.01 r=1.0):\n");
    printf("  %-6s  %-10s  %-10s  %-8s\n",
           "Adim", "Olcum", "Tahmin", "Kazanc");

    for (i = 0U; i < adet; i++)
    {
        float32_t tahmin = Kalman_Guncelle(&f, giris[i]);
        printf("  %-6u  %-10.3f  %-10.3f  %-8.4f\n",
               (uint32_t)i,
               (double)giris[i],
               (double)tahmin,
               (double)f.k);
    }
    /* Kalman kazanci zamanla azalir — modele olan guven artar           */
}

/* ================================================================== */
/* 8. BUTTERWORTH FILTER — FREKANS KESIM FILTRESI                     */
/* ================================================================== */

/*
 * Calisma prensibi:
 *   Maksimum duz gecis bandi — kesim frekansına kadar duz,
 *   sonrasinda keskin dusus. Ikinci dereceden IIR olarak uygulanir.
 *
 * Kullanim alanlari:
 *   Ses filtreleme, titresim analizi, EEG/ECG
 *
 * Avantaj  : duz gecis bandi, keskin kesim
 * Dezavantaj: gecis bandinda faz bozulmasi
 *
 * fc=100Hz, fs=1000Hz, 2. dereceden alcak geciren katsayilari
 * Python: from scipy.signal import butter
 *         b, a = butter(2, 100/500, btype='low')
 */

typedef struct {
    float32_t b0, b1, b2;  /* giris katsayilari                         */
    float32_t a1, a2;      /* geri besleme katsayilari                  */
    float32_t x1, x2;      /* gecmis girisler                           */
    float32_t y1, y2;      /* gecmis cikislar                           */
} Butterworth_t;

static void Butterworth_Init(Butterworth_t * const f)
{
    /* fc=100Hz, fs=1000Hz, 2. dereceden alcak geciren                  */
    f->b0 = 0.0675f;
    f->b1 = 0.1349f;
    f->b2 = 0.0675f;
    f->a1 = -1.1430f;
    f->a2 =  0.4128f;
    f->x1 = 0.0f;
    f->x2 = 0.0f;
    f->y1 = 0.0f;
    f->y2 = 0.0f;
}

static float32_t Butterworth_Guncelle(Butterworth_t * const f, float32_t x)
{
    /* Direct Form II transposed                                         */
    float32_t y = (f->b0 * x) + (f->b1 * f->x1) + (f->b2 * f->x2)
                               - (f->a1 * f->y1) - (f->a2 * f->y2);

    f->x2 = f->x1;
    f->x1 = x;
    f->y2 = f->y1;
    f->y1 = y;

    return y;
}

static void butterworth_ornegi(void)
{
    Butterworth_t f;
    float32_t     giris[] = {0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                              1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    uint8_t       adet    = (uint8_t)(sizeof(giris) / sizeof(giris[0U]));
    uint8_t       i;

    Butterworth_Init(&f);

    printf("  Butterworth 2. Derece (fc=100Hz, fs=1000Hz):\n");
    printf("  %-6s  %-10s  %-10s\n", "Adim", "Giris", "Cikis");

    for (i = 0U; i < adet; i++)
    {
        float32_t cikis = Butterworth_Guncelle(&f, giris[i]);
        printf("  %-6u  %-10.4f  %-10.4f\n",
               (uint32_t)i, (double)giris[i], (double)cikis);
    }
}

/* ================================================================== */
/* 9. ALPHA-BETA FILTER — BASITLESTIRILMIS KALMAN                     */
/* ================================================================== */

/*
 * Calisma prensibi:
 *   Konum ve hiz birlikte tahmin edilir.
 *   Kalman'in sabit kazancli ozel hali — sistem modeli sabit hiz.
 *
 * Kullanim alanlari:
 *   Enkoder konum/hiz takibi, radar hedef takibi
 *
 * Avantaj  : Kalman'dan daha az hesap, konum + hiz tahmini
 * Dezavantaj: sabit kazanc — dinamik ortam icin yetersiz
 *
 * alpha: konum duzeltme katsayisi (0..1)
 * beta : hiz duzeltme katsayisi  (0..1)
 * Kucuk alpha/beta = yumusak ama yavash
 * Buyuk alpha/beta = hizli ama gurultulu
 */

typedef struct {
    float32_t konum;  /* tahmini konum                                   */
    float32_t hiz;    /* tahmini hiz                                     */
    float32_t alpha;  /* konum duzeltme katsayisi                        */
    float32_t beta;   /* hiz duzeltme katsayisi                          */
    float32_t dt;     /* ornekleme periyodu — saniye cinsinden           */
} AlphaBeta_t;

static void AlphaBeta_Init(AlphaBeta_t * const f,
                            float32_t alpha, float32_t beta, float32_t dt)
{
    f->konum = 0.0f;
    f->hiz   = 0.0f;
    f->alpha = alpha;
    f->beta  = beta;
    f->dt    = dt;
}

static float32_t AlphaBeta_Guncelle(AlphaBeta_t * const f, float32_t olcum)
{
    float32_t konum_tahmini;
    float32_t hata;

    /* Tahmin adimi — sabit hiz modeli ile ilerle                       */
    konum_tahmini = f->konum + (f->hiz * f->dt);

    /* Hata hesapla */
    hata = olcum - konum_tahmini;

    /* Guncelleme adimi */
    f->konum = konum_tahmini + (f->alpha * hata);
    f->hiz   = f->hiz        + ((f->beta / f->dt) * hata);

    return f->konum;
}

static void alpha_beta_ornegi(void)
{
    AlphaBeta_t f;
    /* Sabit hizla hareket eden hedef + gurultu                         */
    float32_t giris[] = {10.2f, 20.5f, 29.8f, 40.1f, 50.3f,
                         59.7f, 70.2f, 80.4f, 89.9f, 100.1f};
    uint8_t   adet    = (uint8_t)(sizeof(giris) / sizeof(giris[0U]));
    uint8_t   i;

    /* alpha=0.85 beta=0.005, dt=0.1s (10Hz ornekleme)                 */
    AlphaBeta_Init(&f, 0.85f, 0.005f, 0.1f);

    printf("  Alpha-Beta Filtre (a=0.85 b=0.005):\n");
    printf("  %-6s  %-10s  %-10s  %-10s\n",
           "Adim", "Olcum", "Konum", "Hiz");

    for (i = 0U; i < adet; i++)
    {
        float32_t konum = AlphaBeta_Guncelle(&f, giris[i]);
        printf("  %-6u  %-10.2f  %-10.2f  %-10.2f\n",
               (uint32_t)i,
               (double)giris[i],
               (double)konum,
               (double)f.hiz);
    }
    /* Hiz tahmini ~10 birim/s'ye yaklasir — gercek hiz 100/1s = 10    */
}

/* ================================================================== */
/* 10. DEBOUNCE FILTER — DIJITAL SINYAL KARARLISTIRMA                 */
/* ================================================================== */

/*
 * Calisma prensibi:
 *   Dijital giris N ardisik okumada ayni kalirsa gecerli sayar.
 *   Buton titremesi (50-200ms) ve rotatif enkoder gurultusunu temizler.
 *
 * Kullanim alanlari:
 *   Buton, anahtar, reed kontak, optik sensör dijital cikisi
 *
 * Avantaj  : donanim RC filtre gerektirmez, esik ayarlanabilir
 * Dezavantaj: N*Ts kadar gecikme ekler
 */

#define DEBOUNCE_ESIK  (5U)  /* N ardisik ayni okuma = gecerli           */

typedef struct {
    uint8_t sayac;        /* tutarli okuma sayaci                        */
    uint8_t son_durum;    /* son kararlastirilmis durum                  */
    uint8_t ham_durum;    /* bir onceki ham okuma                        */
} Debounce_t;

static void Debounce_Init(Debounce_t * const f)
{
    f->sayac     = 0U;
    f->son_durum = 0U;
    f->ham_durum = 0U;
}

/* Ham dijital okuma ver, kararlasmis durumu al                          */
static uint8_t Debounce_Guncelle(Debounce_t * const f, uint8_t ham)
{
    if (ham == f->ham_durum) /* ayni deger — sayaci artir               */
    {
        if (f->sayac < DEBOUNCE_ESIK)
        {
            f->sayac++;
        }
        else
        {
            /* esige ulasildi — durum kararlastirildi */
        }

        if (f->sayac >= DEBOUNCE_ESIK) /* esige ulastik — guncelle      */
        {
            f->son_durum = ham;
        }
        else
        {
            /* henuz kararlastirilmadi */
        }
    }
    else /* farkli deger — sayaci sifirla, yeni degeri izle             */
    {
        f->sayac     = 0U;
        f->ham_durum = ham;
    }

    return f->son_durum;
}

static void debounce_ornegi(void)
{
    Debounce_t f;
    /* Buton titremesi simülasyonu — 0=basili degil, 1=basili           */
    uint8_t giris[] = {0U, 0U, 1U, 0U, 1U, 1U, 1U, 1U, 1U, 0U,
                       1U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    uint8_t adet    = (uint8_t)(sizeof(giris) / sizeof(giris[0U]));
    uint8_t i;

    Debounce_Init(&f);

    printf("  Debounce Filtre (esik=%u):\n", (uint32_t)DEBOUNCE_ESIK);
    printf("  %-6s  %-8s  %-8s\n", "Adim", "Ham", "Cikis");

    for (i = 0U; i < adet; i++)
    {
        uint8_t cikis = Debounce_Guncelle(&f, giris[i]);
        printf("  %-6u  %-8u  %-8u\n",
               (uint32_t)i, (uint32_t)giris[i], (uint32_t)cikis);
    }
    /* Titresim sirasinda cikis degismez — esik sayisi kadar okuma gerekir */
}

/* ================================================================== */
/* MAIN                                                                */
/* ================================================================== */

int main(void)
{
    printf("/* 1. MOVING AVERAGE */\n");
    moving_average_ornegi();

    printf("\n/* 2. WEIGHTED MOVING AVERAGE */\n");
    weighted_moving_average_ornegi();

    printf("\n/* 3. EMA */\n");
    ema_ornegi();

    printf("\n/* 4. MEDIAN FILTER */\n");
    median_ornegi();

    printf("\n/* 5. IIR FILTER */\n");
    iir_ornegi();

    printf("\n/* 6. FIR FILTER */\n");
    fir_ornegi();

    printf("\n/* 7. KALMAN FILTER */\n");
    kalman_ornegi();

    printf("\n/* 8. BUTTERWORTH FILTER */\n");
    butterworth_ornegi();

    printf("\n/* 9. ALPHA-BETA FILTER */\n");
    alpha_beta_ornegi();

    printf("\n/* 10. DEBOUNCE FILTER */\n");
    debounce_ornegi();

    return 0;
}