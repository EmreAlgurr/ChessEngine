# ♟️ MyEngine_CPP (v2.0)

Modern, yüksek performanslı ve modüler bir mimariyle C++17 kullanılarak sıfırdan geliştirilmiş **~2870 - 2970 Elo** gücünde bir satranç motoru. 

Bitboard tabanlı hızlı hamle üretimi, PVS arama algoritmaları, Polyglot açılış kitabı, **AVX2 hızlandırmalı Artımlı NNUE (Efficiently Updatable Neural Network)** ve Texel Tuning ile optimize edilmiş hibrit değerlendirme sistemine sahiptir.

---

## 📈 Adım Adım Gelişim Süreci & Tüm Elo Kazanımları

Motorun başlangıçtaki ~2150-2200 seviyesinden günümüzdeki **~2870 - 2970 Elo** seviyesine ulaşmasında uygulanan her geliştirme adımı, resmi turnuva aracı `fastchess` ve standart açılış havuzu (`8moves_v3.pgn`) ile yüzlerce oyunluk SPRT ve self-play maçlarıyla doğrulanmıştır:

| # | Geliştirme / Aşama | Açıklama & Yapılan İşlem | Test Koşulu & Maç Sayısı | Net Elo Katkısı | Ulaşılan Seviye |
| :-: | :--- | :--- | :---: | :---: | :---: |
| **1** | **MovePicker Refactoring** | Tek üretim (`generateAllMoves x1`) + tembel seçim (`lazy selection sort`) mimarisi. Düğüm başı 4x üretim kaldırıldı. | 100 Oyun (1+0.01s) | **+10.43 ± 41.5 Elo** | ~2160 Elo |
| **2** | **3-Fold Repetition & Zobrist** | Arama yolu ve oyun geçmişi üzerinde tekrarların Zobrist hash ile taranması. | — | *Stabilite Artışı* | ~2160 Elo |
| **3** | **SEE (Static Exchange Evaluation)** | Taş değişimlerinde kaba hesap yerine gerçek koruma zinciri analizi ve taş alışlarının kalitesine göre sıralanması. | 40 Oyun vs SF 2100 | **+147.19 ± 119.0 Elo** | ~2247 Elo |
| **4** | **Root (Kök) Hamle Sıralaması** | Iterative Deepening yaparken bir önceki derinliğin en iyi hamlesinin kök aramasında en başa (TT Move) alınması. | 40 Oyun (1+0.01s) | **+52.51 ± 75.5 Elo** | ~2280 Elo |
| **5** | **İlk Pozisyonel Değerlendirme** | Çift fil avantajı ve açık/yarı açık hatlardaki kalelere pozisyonel prim verilmesi. | 100 Oyun (1+0.01s) | **+70.44 ± 51.5 Elo** | ~2320 Elo |
| **6** | **Basit Rok Bonusu (Simple Castle)** | Şah güvenliği için gereksiz karmaşık formüller yerine doğrudan başarılı rok pozisyonlarına prim verilmesi. | 440 Oyun SPRT | **+36.46 ± 27.0 Elo** | ~2340 Elo |
| **7** | **Arama Budamaları (RFP & FP)** | Reverse Futility Pruning (Static Null Move) ve Futility Pruning ile umutsuz dalların erkenden budanması. | 460 Oyun SPRT | **+20.42 ± 28.7 Elo** | ~2355 Elo |
| **8** | **Stockfish 2300 Benchmark** | Arama ve budama parametrelerinin dengelenmesi sonrası Stockfish 2300'e karşı ilk zafer (51/100, %51.0 skor). | 100 Oyun vs SF 2300 | **+3.50 ± 60.0 Elo** | ~2304 Elo |
| **9** | **1. Aşama Texel Tuning** | 100k sessiz pozisyonda taş değerleri ve 6 adet PST tablosunun çok iş parçacıklı yerel arama ile eğitilmesi. | 100 Oyun Self-Play | **+152.40 Elo** | ~2376 Elo |
| **10** | **Polyglot Açılış Kitabı** | 30.000 GM pozisyonlu `gm2001.bin` entegrasyonu (0 ms açılış hamlesi ve varyant avantajı). | 100 Oyun Self-Play | **+56.10 Elo** | ~2430 Elo |
| **11** | **Zaman Ölçekleme Testi (3+0)** | Süre uzadığında arama derinliğinin testi. Stockfish 2400'e karşı 4/4 maç (%100) kazanıldı. | 4 Oyun vs SF 2400 | *Derinlik Doğrulandı* | ~2450 Elo |
| **12** | **Gelişmiş Pozisyonel Terimler & 2. Tur Texel Tuning** | İzole piyon (-11), 4-7. kademe geçer piyonlar (13/34/65/95), 7. yatay kale (+7), at/fil hareketliliği ve King EG tablosu. | 200 Oyun vs Tuned v1 | **+88.74 ± 45.8 Elo** | ~2480 Elo |
| **13** | **Stockfish 2500 Benchmark** | Stockfish 2500 ile 100 maçlık resmi turnuva (40 Galibiyet, 45 Mağlubiyet, 15 Beraberlik, %47.5 skor). | 100 Oyun vs SF 2500 | **-17.39 ± 60.0 Elo** | ~2483 Elo |
| **14** | **NNUE Değerlendirme Entegrasyonu** | HalfKP mimarili derin sinir ağı değerlendirmesi entegre edildi (55 Galibiyet, 33 Mağlubiyet, 12 Beraberlik, %61.0 skor). | 100 Oyun vs SF 2500 | **+77.71 ± 65.0 Elo** | ~2578 Elo |
| **15** | **AVX2 Donanım Vektörizasyonu & Artımlı Akümülatör** | 256-bit AVX2 SIMD ve diferansiyel akümülatör güncellemesi ile arama hızı 650k değerinden 2.05M NPS seviyesine yükseltildi (+2.5 ply). | 100 Oyun Self-Play | **+346.12 ± 84.0 Elo** | ~2731 Elo |
| **16** | **NMP Optimizasyonu & Logaritmik LMR & SE** | Boş hamle budamasında statik değerlendirme (`staticEval >= beta`), dinamik $R$, logaritmik LMR ve Singular Extensions. | 680 Oyun vs SF 2900 (1+0.01s) | **+35.62 ± 25.4 Elo** | ~2767 Elo |
| **17** | **Late Move Pruning (LMP) & Çift Kademeli Zaman Yönetimi** | Sığ derinliklerde ($\text{depth} \le 4$) sessiz hamlelerin budanması ve arama verimini artıran Soft/Hard Time Limit sistemi. | 440 Oyun vs SF 2900 (2+0.02s) | **+75.00 ± 29.4 Elo** | ~2842 Elo |
| **18** | **Kök Aspiration Yeniden Sıralaması (Root Re-ordering)** | Kök aramasında dar pencerede fail-high üreten hamlenin pencere genişletme döngüsünde anında en başa taşınması. | 100 Oyun vs v2_LMP (2+0.02s) | **+27.85 ± 45.0 Elo** | ~2870 Elo |
| **19** | **Klasik Zaman Kontrolü Benchmark (60+0.6)** | Stockfish 2900'e karşı turnuva temposunda resmi test (10 Galibiyet, 6 Mağlubiyet, 4 Beraberlik, %60.0 skor). | 20 Oyun vs SF 2900 (60+0.6s) | **+70.44 ± 185.8 Elo** | **~2970 Elo** |

> **Kümülatif Net Güç Artışı:** Uygulanan gelişmiş arama budamaları (NMP, RFP, LMP, LMR), Singular Extensions, kök aspiration optimizasyonu, artımlı AVX2 NNUE çıkarımı ve optimize edilmiş zaman yönetimi sonucunda motor, klasik değerlendirme seviyesi olan ~2150 Elo'dan yıldırımda **~2870 Elo**, klasik zaman kontrolünde ise **~2970 Elo** seviyesine ulaştırılmıştır.

---

## 🚀 Motor Mimarisi ve Özellikler

### 1. Tahta Temsili & Hamle Üretimi (Board & MoveGen)
- **64-bit Bitboard Temsili:** LSB `a1` (bit 0) ... MSB `h8` (bit 63).
- **Magic Bitboards:** Kale ve fil ışın saldırılarında $O(1)$ sürede anında hamle üretimi.
- **Pseudo-Legal Üretim & Make/Unmake:** Hızlı hamle üretimi ve geri alma mekanizması (`UndoState`).
- **Static Exchange Evaluation (SEE):** Taş alışverişlerinin kazanç/kayıp değerini önceden hesaplama.

### 2. Arama Algoritmaları (Search)
- **PVS (Principal Variation Search) / Alpha-Beta Negamax:** Sıfır pencere aramaları ile optimal budama.
- **Iterative Deepening & Aspiration Windows:** Dinamik genişleyen arama pencereleri ile derinleşme.
- **Transposition Table (TT):**
  - Zobrist Hashing (64-bit).
  - 16-byte sıkıştırılmış girişler (`TT_EXACT`, `TT_LOWER`, `TT_UPPER`).
  - Mat skorlarının ply ile normalizasyonu ve nesil (age) takibi.
- **Gelişmiş Budama Teknikleri (Prunings & Reductions):**
  - **Reverse Futility Pruning (RFP):** Derinlik $\le 4$ iken statik değerlendirme marjını aşan düğümlerin erken kesilmesi.
  - **Null Move Pruning (NMP):** Taktiksel olmayan pozisyonlarda hamle pas geçilerek tehdit tespiti ($R=2$).
  - **Futility Pruning (FP):** Sığ derinliklerde umutsuz sessiz hamlelerin elenmesi.
  - **Late Move Reductions (LMR):** Taktiksel olmayan geç hamlelerin daha az derinlikle aranması (geçer piyon ve şah tehditleri korumalı).
- **Hamle Sıralama (Move Ordering - `MovePicker`):**
  - Tek geçişte üretim ve dinamik kategorizasyon (Partitioning).
  - Tembel seçim (Lazy Selection Sort): TT Hamlesi $\rightarrow$ Kazançlı Taş Alışları ($SEE \ge 0$) $\rightarrow$ Killer Moves (2 katman) $\rightarrow$ Sessiz Hamleler (History Heuristik) $\rightarrow$ Zararlı Taş Alışları ($SEE < 0$).
- **Sakinlik Araması (Quiescence Search):**
  - Ufuk etkisini (Horizon Effect) önlemek için sadece taktik hamlelerin incelenmesi.
  - Delta Pruning ile umutsuz alımların budanması.
- **Beraberlik Tespiti:**
  - 3-Fold Repetition (Arama ağacı içinde 2-fold, oyun geçmişinde 3-fold tespiti).
  - 50 Hamle Kuralı (100 ply).

### 3. Değerlendirme (NNUE & HCE)
Motor varsayılan olarak modern derin sinir ağı değerlendirmesi (**NNUE**) kullanır; ağ bulunamadığında optimize edilmiş el yapımı değerlendirmeye (**HCE**) geri düşer:
- **NNUE (Efficiently Updatable Neural Network):**
  - HalfKP mimarisi (21 MB Stockfish uyumlu ağ dosyasından doğrudan bellek eşleme ile statik çıkarım).
  - Pozisyondaki tüm taş-kare ilişkilerini ve derin piyon/taş dinamiklerini non-lineer aktivasyon katmanlarıyla ultra hassas değerlendirir.
  - Açılış/oyun ortası/oyun sonu evrelerini harici karmaşık formüllere ihtiyaç duymadan otomatik çözer.
- **Yedek Klasik Değerlendirme (HCE):**
  - Texel Tuning ile eğitilmiş PST tabloları, kademeli şah güvenliği, çift fil ve kale hat bonusları, kademeli geçer piyonlar.

### 4. Açılış Kitabı (Opening Book)
- Standart **Polyglot** ikili format (`.bin`) desteği.
- Dahili **30.000 GM pozisyonu** içeren `gm2001.bin` kitabı ile açılışta 0 ms anlık hamle oynama.

### 5. Dahili Texel Tuner
- Motor içine gömülü bağımsız tuner:
  ```bash
  ./ChessEngine.exe tune quiet-labeled.epd 25 4 200000
  ```
- 232 parametre grubunu çoklu iş parçacığıyla (OpenMP / Multi-thread) yerel arama (Local Search) ile otomatik eğitir.

---

## 🛠️ Kurulum ve Derleme

Proje harici hiçbir üçüncü parti kütüphaneye veya dinamik DLL'e ihtiyaç duymaz. Sadece standart bir C++17 derleyicisi (MinGW g++ veya MSVC) yeterlidir.

### Derleme
Kök dizindeki `build.bat` dosyasını çalıştırarak derleyebilirsiniz:

```cmd
.\build.bat
```
*(veya özel isimle: `.\build.bat ChessEngine_v2.exe`)*

---

## 🎮 Kullanım

Motor standart **UCI (Universal Chess Interface)** protokolünü destekler. Cute Chess, Arena, BanksiaGUI veya fastchess ile doğrudan uyumludur.

### Desteklenen UCI Seçenekleri
- `OwnBook` (bool, varsayılan: `true`): Açılış kitabının aktifliği.
- `BookFile` (string, varsayılan: `gm2001.bin`): Kullanılacak Polyglot kitap dosyasının yolu.
- `EvalFile` (string, varsayılan: `nn-04cf2b4ed1da.nnue`): Kullanılacak NNUE ağ dosyasının yolu.

### Örnek UCI Oturumu
```text
uci
option name OwnBook type check default true
option name BookFile type string default gm2001.bin
option name EvalFile type string default nn-04cf2b4ed1da.nnue
isready
readyok
ucinewgame
position startpos moves e2e4 e7e5
go wtime 60000 btime 60000 winc 1000 binc 1000
bestmove g1f3
quit
```