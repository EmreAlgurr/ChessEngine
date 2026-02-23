# ♟️ MyEngine_CPP

Modern ve modüler bir mimariyle C++ kullanılarak sıfırdan geliştirilmiş bir satranç motoru. Bu proje, geleneksel satranç programlama tekniklerini güncel algoritmalarla birleştirmeyi amaçlar.

## 🚀 Özellikler

- **Arama Algoritması**: Alpha-Beta budamalı (Pruning) **Negamax** arama sistemi.
- **Iterative Deepening**: Zaman kontrolüne duyarlı, derinliği dinamik olarak artıran arama yapısı.
- **Quiescence Search (Sakinlik Araması)**: "Ufuk Etkisi"ni (Horizon Effect) önlemek için taktiksel taş yeme dizilerini sonuna kadar analiz eder.
- **Move Ordering**: Arama ağacını verimli budamak için **MVV-LVA** (Most Valuable Victim - Least Valuable Attacker) hamle sıralama tekniği.
- **Board Representation**: Hızlı hamle üretimi için **120-kare mailbox** yapısı.
- **Değerlendirme (Evaluation)**: Materyal dengesi ve **Piece-Square Tables (PST)** tabanlı konum analizi.
- **UCI Protokolü**: Stockfish gibi popüler motorlarla aynı dili konuşur; Arena, Cute Chess ve fastchess gibi GUI'lerde sorunsuz çalışır.

## 🛠️ Kurulum ve Derleme

Proje herhangi bir harici kütüphane gerektirmez, sadece standart C++ derleyicisi (g++ veya MSVC) yeterlidir.

### Windows
Kök dizindeki `build.bat` dosyasını çalıştırarak `ChessEngine.exe` dosyasını oluşturabilirsiniz:
```cmd
./build.bat
```

## 🎮 Kullanım

Motoru terminal üzerinden UCI komutlarıyla veya bir GUI (Arayüz) üzerinden kullanabilirsiniz.

**Örnek UCI Akışı:**
```text
uci
isready
position startpos
go depth 5
```

## 📈 Gelecek Planları
- [ ] Zobrist Hashing ve Transposition Table (TT) entegrasyonu.
- [ ] Bitboard mimarisine geçiş.
- [ ] Daha gelişmiş Piyon yapısı analizi ve Şah güvenliği değerlendirmesi.