# ♟️ MyEngine_CPP (v2.0)

Modern, yüksek performanslı ve modüler bir mimariyle C++17 kullanılarak sıfırdan geliştirilmiş **~2483 Elo** gücünde bir satranç motoru. 

Bitboard tabanlı hızlı hamle üretimi, PVS arama algoritmaları, Polyglot açılış kitabı ve dahili çok iş parçacıklı **Texel Tuning** ile optimize edilmiş el yapımı değerlendirme (HCE) sistemine sahiptir.

---

## 🏆 Başarılar & Performans Ölçümleri

Resmi turnuva aracı `fastchess` ile standart açılış havuzu (`8moves_v3.pgn`) kullanılarak yapılan maç sonuçları:

| Rakip / Test | Zaman Kontrolü | Skor / Sonuç | Performans / Net Kazanç |
| :--- | :--- | :--- | :--- |
| **vs Stockfish 2300** | 1+0.01s | 51.0 / 100 (%51.0) | **+3.5 Elo** (Galibiyet) |
| **1. Tur Texel Tuning** | 1+0.01s | Self-play | **+152.4 Elo** sıçrama |
| **vs Stockfish 2450** | 1+0.01s | 40.5 / 100 (%40.5) | **2376 Elo** |
| **Polyglot Kitap Entegrasyonu** | 1+0.01s | Self-play | **+56.1 Elo** |
| **2. Tur Texel Tuning (Pozisyonel)** | 1+0.01s | 125.0 / 200 (%62.5) | **+88.74 Elo** (%100 LOS) |
| **vs Stockfish 2500** | 1+0.01s | 47.5 / 100 (40W - 45L - 15D) | **~2483 ELO** |

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