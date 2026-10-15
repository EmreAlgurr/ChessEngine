# ♟️ MyEngine_CPP (v2.0)

Modern, yüksek performanslı ve modüler bir mimariyle C++17 kullanılarak sıfırdan geliştirilmiş **~2578 Elo** gücünde bir satranç motoru. 

Bitboard tabanlı hızlı hamle üretimi, PVS arama algoritmaları, Polyglot açılış kitabı, **NNUE (Efficiently Updatable Neural Network)** ve Texel Tuning ile optimize edilmiş el yapımı değerlendirme (HCE) sistemine sahiptir.

---

## 📈 Adım Adım Gelişim Süreci & Tüm Elo Kazanımları

Motorun başlangıçtaki ~2150-2200 seviyesinden günümüzdeki **~2578 Elo** seviyesine ulaşmasında uygulanan her geliştirme adımı, resmi turnuva aracı `fastchess` ve standart açılış havuzu (`8moves_v3.pgn`) ile yüzlerce oyunluk SPRT ve self-play maçlarıyla doğrulanmıştır:

| # | Geliştirme / Aşama | Açıklama & Yapılan İşlem | Test Türü & Maç Sayısı | Ölçülen Net Elo |
| :-: | :--- | :--- | :---: | :---: |
| **1** | **MovePicker Refactoring** | Tek üretim (`generateAllMoves x1`) + tembel seçim (`lazy selection sort`) mimarisi. Düğüm başı 4x üretim kalktı. | 100 Oyun (1+0.01s) | **+10.43 ± 41.5 Elo** |
| **2** | **3-Fold Repetition & Zobrist** | Arama yolu ve oyun geçmişi üzerinde tekrarların Zobrist hash ile taranması. | — | *Stabilite Artışı* |
| **3** | **SEE (Static Exchange Evaluation)** | Taş değişimlerinde kaba hesap yerine gerçek koruma zinciri analizi ve taş alışlarının kalitesine göre sıralanması. | vs SF 2100 (40 Oyun) | **+147.19 ± 119 Elo** |
| **4** | **Root (Kök) Hamle Sıralaması** | Iterative Deepening yaparken bir önceki derinliğin en iyi hamlesinin kök aramasında en başa (TT Move) alınması. | 40 Oyun (1+0.01s) | **+52.51 ± 75.5 Elo** |
| **5** | **İlk Pozisyonel Değerlendirme** | Çift fil avantajı ve açık/yarı açık hatlardaki kalelere pozisyonel prim verilmesi. | 100 Oyun (1+0.01s) | **+70.44 ± 51.5 Elo** |
| **6** | **Basit Rok Bonusu (Simple Castle)** | Şah güvenliği için gereksiz karmaşık formüller yerine doğrudan başarılı rok pozisyonlarına prim verilmesi. | 440 Oyun SPRT | **+36.46 ± 27.0 Elo** |
| **7** | **Arama Budamaları (RFP & FP)** | Reverse Futility Pruning (Static Null Move) ve Futility Pruning ile umutsuz dalların erkenden budanması. | 460 Oyun SPRT | **+20.42 ± 28.7 Elo** |
| **8** | **Stockfish 2300 Barajı** | Arama ve budama parametrelerinin dengelenmesi sonrası Stockfish 2300'e karşı ilk zafer (51/100, %51). | 100 Oyun vs SF 2300 | **+3.5 Elo (2303 Elo)** |
| **9** | **1. Aşama Texel Tuning** | 100k sessiz pozisyonda taş değerleri ve 6 adet PST tablosunun çok iş parçacıklı yerel arama ile eğitilmesi. | Self-Play (100 Oyun) | **+152.4 Elo** *(SF 2450'ye karşı 2376 Elo)* |
| **10** | **Polyglot Açılış Kitabı** | 30.000 GM pozisyonlu `gm2001.bin` entegrasyonu (0 ms açılış hamlesi ve varyant avantajı). | Self-Play (100 Oyun) | **+56.1 Elo** |
| **11** | **Zaman Ölçekleme Testi (3+0)** | Süre uzadığında arama derinliğinin testi. Stockfish 2400'e karşı 4/4 maç (%100) kazanıldı. | 4 Maç vs SF 2400 | *Derin Arama Doğrulandı* |
| **12** | **Gelişmiş Pozisyonel Terimler & 2. Tur Texel Tuning** | İzole piyon (-11), 4-7. kademe geçer piyonlar (13/34/65/95), 7. yatay kale (+7), at/fil hareketliliği (+2/+4) ve King EG tablosu eklenip 200k pozisyonda 25 epoch eğitildi. | 200 Oyun vs Tuned v1 | **+88.74 ± 45.8 Elo** (%100 LOS) |
| **13** | **Stockfish 2500 Benchmark** | Stockfish 2500 ile 100 maçlık resmi turnuva:<br>**40 Galibiyet, 45 Mağlubiyet, 15 Beraberlik (%47.5 Skor)** | 100 Oyun vs SF 2500 | **~2483 ELO** *(Sadece -17.39 Elo fark)* |
| **14** | **NNUE Değerlendirme Entegrasyonu** | HalfKP mimarili derin sinir ağı değerlendirmesi entegre edildi. Stockfish 2500'e karşı:<br>**55 Galibiyet, 33 Mağlubiyet, 12 Beraberlik (%61.0 Skor)** | 100 Oyun vs SF 2500 | **+77.71 ± 65 Elo (~2578 Elo)** |

> **Kümülatif Net Güç Artışı:** Uygulanan tüm arama budamaları, taş/PST optimizasyonları, açılış kitabı ve NNUE sinir ağı değerlendirmesi motoru ~2150 Elo'dan alıp **~2578 Elo** seviyesine ulaştırmış, Stockfish 2500'e 22 galibiyet fark atarak resmi olarak geride bırakmıştır.

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

---

## 📈 Gelecek Planları (Roadmap)
- [x] **NNUE (Neural Network Evaluation)** mimarisine geçiş. *(Tamamlandı: ~2578 Elo, SF 2500 karşısında %61 galibiyet).*
- [ ] **Artımlı Akümülatör (Incremental NNUE Accumulator):** `makeMove`/`undoMove` içinde fark hesaplama ile değerlendirme maliyetini düşürüp NPS'i 2-3 katına çıkarma.
- [ ] **Singular Extensions (SE):** Pozisyonda tek kurtarıcı taktik hamle olduğunda aramayı derinleştirme (+60-80 Elo).
- [ ] **Continuation & Capture History:** Hamle sıralamasını modern seviyeye taşıma.
- [ ] **Late Move Pruning (LMP):** Derin aramalarda umutsuz sessiz hamleleri tamamen eleme.