# ♟️ MyEngine_CPP (v2.0)

Modern, yüksek performanslı ve modüler bir mimariyle C++17 kullanılarak sıfırdan geliştirilmiş **~2483 Elo** gücünde bir satranç motoru. 

Bitboard tabanlı hızlı hamle üretimi, PVS arama algoritmaları, Polyglot açılış kitabı ve dahili çok iş parçacıklı **Texel Tuning** ile optimize edilmiş el yapımı değerlendirme (HCE) sistemine sahiptir.

---

## 📈 Adım Adım Gelişim Süreci & Tüm Elo Kazanımları

Motorun başlangıçtaki ~2150-2200 seviyesinden günümüzdeki **~2483 Elo** seviyesine ulaşmasında uygulanan her geliştirme adımı, resmi turnuva aracı `fastchess` ve standart açılış havuzu (`8moves_v3.pgn`) ile yüzlerce oyunluk SPRT ve self-play maçlarıyla doğrulanmıştır:

| # | Geliştirme / Eklenen Özellik | Açıklama & Yapılan İşlem | Ölçülen Net Katkı | Ulaşılan Seviye |
| :-: | :--- | :--- | :---: | :---: |
| **1** | **MovePicker & Hamle Sıralama** | Tek üretim (`generateAllMoves x1`) + tembel seçim (`lazy selection`). TT Hamlesi, SEE tabanlı taktik ayırımı ve 2 katmanlı Killer Moves. | *NPS ve Arama Kararlılığı* | ~2200 Elo |
| **2** | **İlk Büyük Baraj (SF 2300)** | Budamaların (NMP, RFP, FP, LMR) dengelenmesiyle Stockfish 2300 karşısında test edildi (51.0/100, %51). | **+3.5 Elo vs SF 2300** | **2303 Elo** |
| **3** | **1. Aşama Texel Tuning** | Dahili tuner ile 100k sessiz pozisyonda taş değerleri, çift fil, açık hat kale ve 6 adet PST tablosu yerel arama ile optimize edildi. | **+152.4 Elo** (Self-Play) | **2376 Elo** *(SF 2450'ye karşı)* |
| **4** | **Polyglot Açılış Kitabı** | 30.000 GM pozisyonlu `gm2001.bin` binary açılış kitabı entegrasyonu. Açılışta 0 ms hamle oynayarak tuzaklardan kaçındı ve süre avantajı sağladı. | **+56.1 Elo** (Self-Play) | **~2430 Elo** |
| **5** | **Zaman Ölçekleme Testi (3+0)** | Süre uzadığında taktiksel derinliğin test edilmesi. Stockfish 2400'e karşı 4/4 maç (%100) kazanıldı. | *Derin Arama Gücü Doğrulandı* | — |
| **6** | **Gelişmiş Pozisyonel Terimler** | Değerlendirme fonksiyonuna 5 yeni stratejik terim eklendi:<br>• **İzole Piyon Cezası** (-11)<br>• **Kademeli Geçer Piyon Bonusları** (4-7. yataylar: 13 / 34 / 65 / 95)<br>• **7. Yatayda Kale Bonusu** (+7)<br>• **Taş Hareketliliği (Safe Mobility):** At (+2) ve Fil (+4)<br>• **Oyun Sonu Şah Tablosu (King EG):** Vezirler kesilince merkeze hücum | *Tuning Öncesi Hazırlık* | — |
| **7** | **2. Aşama Texel Tuning (200k Pozisyon)** | 232 parametre grubu, 200.000 GM pozisyonunda 25 epoch boyunca eğitildi. Hata oranı (MSE) `0.0717`den `0.0701`e düşürüldü. Eski Tuned motora karşı 200 maçta 107W - 57L - 36D (%62.5). | **+88.74 Elo** (%100 LOS) | **~2480+ Elo** |
| **8** | **Stockfish 2500 Benchmark Testi** | Stockfish 2500 ile 100 oyunluk resmi turnuva maçı (1+0.01):<br>**40 Galibiyet, 45 Mağlubiyet, 15 Beraberlik (47.5 / 100 puan - %47.5)** | Sadece **-17.39 Elo** fark | **~2483 ELO** |

> **Kümülatif Net Güç Artışı:** Yapılan optimizasyonlar, açılış kitabı ve iki aşamalı Texel Tuning süreci motora **+297'den fazla net Elo** kazandırmış ve motoru doğrudan Stockfish 2500 seviyesine çıkarmıştır.

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

### 3. Değerlendirme (Handcrafted Evaluation - HCE)
Tamamı **Texel Tuning** yöntemiyle 200.000 GM pozisyonu üzerinden 25 epoch boyunca optimize edilmiştir:
- **Temel Materyal ve PST Tabloları:** Piyon, At, Fil, Kale, Vezir ve Şah için optimize kare ağırlıkları.
- **Tapered Şah Tablosu:** Oyun ortası güvenliği (MG) ile oyun sonu merkezileşmesi (EG) arasında geçiş.
- **Çift Fil Bonusu (Bishop Pair):** Açık tahtada çift fil avantajı.
- **Kale Pozisyonel Bonusları:** Açık hat, yarı-açık hat ve **7. yataydaki kale** bonusu.
- **Piyon Yapısı:**
  - İzole piyon cezası.
  - Kademeli geçer piyon bonusları (4., 5., 6. ve 7. yataylar için artan değerler).
- **Taş Hareketliliği (Mobility):** At ve fillerin rakip piyon tehdidi altında olmayan güvenli kare kontrolü.

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

Proje harici hiçbir üçüncü parti kütüphaneye ihtiyaç duymaz. Sadece standart bir C++17 derleyicisi (MinGW g++ veya MSVC) yeterlidir.

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

### Örnek UCI Oturumu
```text
uci
option name OwnBook type check default true
option name BookFile type string default gm2001.bin
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
- [ ] Tahta temsili hızlandırması ($O(1)$ Mailbox dizi erişimi ile NPS artışı).
- [ ] Transposition Table için derinlik/nesil öncelikli (Depth-preferred) değiştirme politikası.
- [ ] Tüm taşlar için kademeli (Tapered MG/EG) değerlendirme ve piyon kalkanı (Pawn Shield) şah güvenliği.
- [ ] NNUE (Neural Network Evaluation) mimarisine geçiş.