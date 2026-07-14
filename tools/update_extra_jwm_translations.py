#!/usr/bin/env python3
from pathlib import Path
from po_tools import update_po
from update_desktop_translations import MESSAGES

RAW = {}
def add(lang, text):
    values = [line.strip() for line in text.strip().splitlines()]
    if len(values) != len(MESSAGES):
        raise SystemExit(f'{lang}: expected {len(MESSAGES)}, got {len(values)}')
    RAW[lang] = dict(zip(MESSAGES, values))

add('da', r'''
Kunne ikke åbne X-skærmen
EssoraWM-skrivebordsstyring
Ingen programstartere blev fundet
Vælg et program, der skal føjes til skrivebordet
Flyt drevikoner frit på skrivebordet, eller arranger dem her
Standardindstillingerne for drevikoner er gendannet; tryk på Anvend
Ingen programmer matcher søgningen
Kunne ikke oprette PuppyPin-konfigurationsmappen
Kunne ikke oprette PuppyPin
Dette program findes allerede på skrivebordet
PuppyPin er ugyldig: </pinboard> blev ikke fundet
Der er ikke nok hukommelse til at opdatere PuppyPin
Kunne ikke skrive PuppyPin
Føjet til skrivebordet. EssoraWM gemmer den nye placering, når du flytter den
Dette program findes ikke på skrivebordet
Fjernet fra skrivebordet
Kunne ikke gemme indstillingerne for drevikoner
Indstillingerne for drevikoner er anvendt
Skjulte drev er synlige igen
Indstillingerne for drevikoner er ændret; tryk på Anvend
Programmer og indbyggede drevikoner
Programmer
Drevikoner
Søg
Skriv for at filtrere programmer
Tilføj
Fjern
Luk
Synlighed
Vis drevikoner
Interne drev
Flytbare drev
Netværksdrev
Vis etiketter
Etiketramme
Omvendt rækkefølge
Du kan også trække hvert drevikon frit på skrivebordet.
Anvend arrangerer alle drev efter det valgte layout.
Layout
Vandret
Lodret
Vandret placering
Venstre
Centreret
Højre
Lodret placering
Øverst
Nederst
Ikonstørrelse
Vandret afstand
Lodret afstand
Vandret forskydning
Lodret forskydning
Vis skjulte drev
Anvend
Anvendt
Standarder
Åbn
Afmontér
Montér
Fjern fra skrivebordet
Drev %s
''')

add('nl', r'''
Kon het X-scherm niet openen
EssoraWM-bureaubladbeheer
Geen programmastarters gevonden
Selecteer een toepassing om aan het bureaublad toe te voegen
Verplaats schijfpictogrammen vrij op het bureaublad of rangschik ze hier
Standaardinstellingen voor schijfpictogrammen hersteld; klik op Toepassen
Geen toepassingen komen overeen met de zoekopdracht
Kon de configuratiemap van PuppyPin niet maken
Kon PuppyPin niet maken
Deze toepassing staat al op het bureaublad
PuppyPin is ongeldig: </pinboard> is niet gevonden
Onvoldoende geheugen om PuppyPin bij te werken
Kon PuppyPin niet schrijven
Toegevoegd aan het bureaublad. EssoraWM slaat de nieuwe positie op wanneer u het verplaatst
Deze toepassing staat niet op het bureaublad
Verwijderd van het bureaublad
Kon de instellingen voor schijfpictogrammen niet opslaan
Instellingen voor schijfpictogrammen toegepast
Verborgen schijven zijn weer zichtbaar
Instellingen voor schijfpictogrammen zijn gewijzigd; klik op Toepassen
Toepassingen en ingebouwde schijfpictogrammen
Toepassingen
Schijfpictogrammen
Zoeken
Typ om toepassingen te filteren
Toevoegen
Verwijderen
Sluiten
Zichtbaarheid
Schijfpictogrammen tonen
Interne schijven
Verwisselbare schijven
Netwerkschijven
Labels tonen
Labelkader
Volgorde omkeren
U kunt elk schijfpictogram ook vrij over het bureaublad slepen.
Toepassen rangschikt alle schijven volgens de gekozen indeling.
Indeling
Horizontaal
Verticaal
Horizontale positie
Links
Midden
Rechts
Verticale positie
Boven
Onder
Pictogramgrootte
Horizontale afstand
Verticale afstand
Horizontale verschuiving
Verticale verschuiving
Verborgen schijven tonen
Toepassen
Toegepast
Standaardwaarden
Openen
Ontkoppelen
Aankoppelen
Van bureaublad verwijderen
Volume %s
''')

add('pl', r'''
Nie można otworzyć ekranu X
Menedżer pulpitu EssoraWM
Nie znaleziono aktywatorów programów
Wybierz program do dodania na pulpit
Przesuwaj ikony dysków swobodnie po pulpicie lub ułóż je tutaj
Przywrócono domyślne ustawienia ikon dysków; naciśnij Zastosuj
Brak programów pasujących do wyszukiwania
Nie można utworzyć katalogu konfiguracji PuppyPin
Nie można utworzyć PuppyPin
Ten program znajduje się już na pulpicie
PuppyPin jest nieprawidłowy: nie znaleziono </pinboard>
Za mało pamięci, aby zaktualizować PuppyPin
Nie można zapisać PuppyPin
Dodano na pulpit. EssoraWM zapisze nowe położenie po przesunięciu
Tego programu nie ma na pulpicie
Usunięto z pulpitu
Nie można zapisać ustawień ikon dysków
Zastosowano ustawienia ikon dysków
Ukryte dyski są ponownie widoczne
Ustawienia ikon dysków zostały zmienione; naciśnij Zastosuj
Programy i natywne ikony dysków
Programy
Ikony dysków
Szukaj
Pisz, aby filtrować programy
Dodaj
Usuń
Zamknij
Widoczność
Pokaż ikony dysków
Dyski wewnętrzne
Dyski wymienne
Dyski sieciowe
Pokaż etykiety
Ramka etykiety
Odwróć kolejność
Każdą ikonę dysku można również swobodnie przeciągać po pulpicie.
Zastosuj układa wszystkie dyski według wybranego układu.
Układ
Poziomo
Pionowo
Położenie poziome
Lewa
Środek
Prawa
Położenie pionowe
Góra
Dół
Rozmiar ikony
Odstęp poziomy
Odstęp pionowy
Przesunięcie poziome
Przesunięcie pionowe
Pokaż ukryte dyski
Zastosuj
Zastosowano
Domyślne
Otwórz
Odmontuj
Zamontuj
Usuń z pulpitu
Wolumin %s
''')

add('tr', r'''
X ekranı açılamadı
EssoraWM Masaüstü Yöneticisi
Uygulama başlatıcısı bulunamadı
Masaüstüne eklenecek bir uygulama seçin
Sürücü simgelerini masaüstünde serbestçe taşıyın veya burada düzenleyin
Varsayılan sürücü simgesi ayarları geri yüklendi; Uygula düğmesine basın
Aramayla eşleşen uygulama yok
PuppyPin yapılandırma dizini oluşturulamadı
PuppyPin oluşturulamadı
Bu uygulama zaten masaüstünde
PuppyPin geçersiz: </pinboard> bulunamadı
PuppyPin’i güncellemek için yeterli bellek yok
PuppyPin yazılamadı
Masaüstüne eklendi. Taşıdığınızda EssoraWM yeni konumu kaydedecek
Bu uygulama masaüstünde değil
Masaüstünden kaldırıldı
Sürücü simgesi ayarları kaydedilemedi
Sürücü simgesi ayarları uygulandı
Gizli sürücüler yeniden görünür
Sürücü simgesi ayarları değişti; Uygula düğmesine basın
Uygulamalar ve yerel sürücü simgeleri
Uygulamalar
Sürücü simgeleri
Ara
Uygulamaları süzmek için yazın
Ekle
Kaldır
Kapat
Görünürlük
Sürücü simgelerini göster
Dahili sürücüler
Çıkarılabilir sürücüler
Ağ sürücüleri
Etiketleri göster
Etiket çerçevesi
Sıralamayı ters çevir
Her sürücü simgesini masaüstünde serbestçe sürükleyebilirsiniz.
Uygula, tüm sürücüleri seçilen düzene göre yerleştirir.
Düzen
Yatay
Dikey
Yatay konum
Sol
Orta
Sağ
Dikey konum
Üst
Alt
Simge boyutu
Yatay aralık
Dikey aralık
Yatay kaydırma
Dikey kaydırma
Gizli sürücüleri göster
Uygula
Uygulandı
Varsayılanlar
Aç
Ayır
Bağla
Masaüstünden kaldır
Birim %s
''')

add('uk', r'''
Не вдалося відкрити екран X
Менеджер стільниці EssoraWM
Не знайдено засобів запуску програм
Виберіть програму для додавання на стільницю
Вільно переміщуйте піктограми дисків стільницею або впорядкуйте їх тут
Типові налаштування піктограм дисків відновлено; натисніть «Застосувати»
Немає програм, що відповідають пошуку
Не вдалося створити каталог налаштувань PuppyPin
Не вдалося створити PuppyPin
Ця програма вже є на стільниці
PuppyPin некоректний: </pinboard> не знайдено
Недостатньо пам’яті для оновлення PuppyPin
Не вдалося записати PuppyPin
Додано на стільницю. EssoraWM збереже нове положення після переміщення
Цієї програми немає на стільниці
Видалено зі стільниці
Не вдалося зберегти налаштування піктограм дисків
Налаштування піктограм дисків застосовано
Приховані диски знову видимі
Налаштування піктограм дисків змінено; натисніть «Застосувати»
Програми та вбудовані піктограми дисків
Програми
Піктограми дисків
Пошук
Введіть текст для фільтрування програм
Додати
Видалити
Закрити
Видимість
Показувати піктограми дисків
Внутрішні диски
Знімні диски
Мережеві диски
Показувати підписи
Рамка підпису
Зворотний порядок
Кожну піктограму диска також можна вільно перетягувати стільницею.
«Застосувати» впорядкує всі диски за вибраною схемою.
Розташування
Горизонтально
Вертикально
Горизонтальне положення
Ліворуч
По центру
Праворуч
Вертикальне положення
Вгорі
Внизу
Розмір піктограми
Горизонтальний інтервал
Вертикальний інтервал
Горизонтальний зсув
Вертикальний зсув
Показати приховані диски
Застосувати
Застосовано
Типові значення
Відкрити
Відмонтувати
Змонтувати
Видалити зі стільниці
Том %s
''')

add('lt', r'''
Nepavyko atverti X ekrano
EssoraWM darbalaukio tvarkytuvė
Programų paleidiklių nerasta
Pasirinkite programą, kurią norite pridėti prie darbalaukio
Laisvai perkelkite diskų piktogramas darbalaukyje arba išdėstykite jas čia
Atkurti numatytieji diskų piktogramų nustatymai; spustelėkite Taikyti
Paiešką atitinkančių programų nėra
Nepavyko sukurti PuppyPin konfigūracijos aplanko
Nepavyko sukurti PuppyPin
Ši programa jau yra darbalaukyje
PuppyPin netinkamas: </pinboard> nerastas
Nepakanka atminties PuppyPin atnaujinti
Nepavyko įrašyti PuppyPin
Pridėta prie darbalaukio. Perkėlus EssoraWM išsaugos naują vietą
Šios programos darbalaukyje nėra
Pašalinta iš darbalaukio
Nepavyko išsaugoti diskų piktogramų nustatymų
Diskų piktogramų nustatymai pritaikyti
Paslėpti diskai vėl matomi
Diskų piktogramų nustatymai pakeisti; spustelėkite Taikyti
Programos ir savosios diskų piktogramos
Programos
Diskų piktogramos
Ieškoti
Rašykite programoms filtruoti
Pridėti
Pašalinti
Užverti
Matomumas
Rodyti diskų piktogramas
Vidiniai diskai
Keičiamieji diskai
Tinklo diskai
Rodyti etiketes
Etiketės rėmelis
Atvirkštinė tvarka
Kiekvieną disko piktogramą taip pat galite laisvai vilkti darbalaukyje.
Taikyti išdėsto visus diskus pagal pasirinktą maketą.
Išdėstymas
Horizontaliai
Vertikaliai
Horizontali padėtis
Kairėje
Centre
Dešinėje
Vertikali padėtis
Viršuje
Apačioje
Piktogramos dydis
Horizontalus tarpas
Vertikalus tarpas
Horizontalus poslinkis
Vertikalus poslinkis
Rodyti paslėptus diskus
Taikyti
Pritaikyta
Numatytieji
Atverti
Atjungti
Prijungti
Pašalinti iš darbalaukio
Tomas %s
''')

add('th', r'''
ไม่สามารถเปิดจอแสดงผล X ได้
ตัวจัดการเดสก์ท็อป EssoraWM
ไม่พบตัวเรียกใช้โปรแกรม
เลือกโปรแกรมที่จะเพิ่มลงบนเดสก์ท็อป
ย้ายไอคอนไดรฟ์บนเดสก์ท็อปได้อย่างอิสระ หรือจัดเรียงที่นี่
คืนค่าการตั้งค่าไอคอนไดรฟ์เริ่มต้นแล้ว กด ใช้
ไม่มีโปรแกรมที่ตรงกับการค้นหา
ไม่สามารถสร้างไดเรกทอรีการตั้งค่า PuppyPin ได้
ไม่สามารถสร้าง PuppyPin ได้
โปรแกรมนี้อยู่บนเดสก์ท็อปแล้ว
PuppyPin ไม่ถูกต้อง: ไม่พบ </pinboard>
หน่วยความจำไม่เพียงพอสำหรับอัปเดต PuppyPin
ไม่สามารถเขียน PuppyPin ได้
เพิ่มลงบนเดสก์ท็อปแล้ว EssoraWM จะบันทึกตำแหน่งใหม่เมื่อคุณย้าย
โปรแกรมนี้ไม่ได้อยู่บนเดสก์ท็อป
นำออกจากเดสก์ท็อปแล้ว
ไม่สามารถบันทึกการตั้งค่าไอคอนไดรฟ์ได้
ใช้การตั้งค่าไอคอนไดรฟ์แล้ว
ไดรฟ์ที่ซ่อนอยู่แสดงอีกครั้ง
การตั้งค่าไอคอนไดรฟ์เปลี่ยนแล้ว กด ใช้
โปรแกรมและไอคอนไดรฟ์แบบเนทีฟ
โปรแกรม
ไอคอนไดรฟ์
ค้นหา
พิมพ์เพื่อกรองโปรแกรม
เพิ่ม
นำออก
ปิด
การแสดงผล
แสดงไอคอนไดรฟ์
ไดรฟ์ภายใน
ไดรฟ์แบบถอดได้
ไดรฟ์เครือข่าย
แสดงป้ายชื่อ
กรอบป้ายชื่อ
กลับลำดับ
คุณสามารถลากไอคอนไดรฟ์แต่ละอันบนเดสก์ท็อปได้อย่างอิสระ
ใช้ จะจัดเรียงไดรฟ์ทั้งหมดตามรูปแบบที่เลือก
รูปแบบ
แนวนอน
แนวตั้ง
ตำแหน่งแนวนอน
ซ้าย
กึ่งกลาง
ขวา
ตำแหน่งแนวตั้ง
บน
ล่าง
ขนาดไอคอน
ระยะห่างแนวนอน
ระยะห่างแนวตั้ง
ระยะเยื้องแนวนอน
ระยะเยื้องแนวตั้ง
แสดงไดรฟ์ที่ซ่อน
ใช้
ใช้แล้ว
ค่าเริ่มต้น
เปิด
ยกเลิกการเมานต์
เมานต์
นำออกจากเดสก์ท็อป
โวลุ่ม %s
''')

add('ka', r'''
X ეკრანის გახსნა ვერ მოხერხდა
EssoraWM სამუშაო მაგიდის მმართველი
პროგრამების გამშვები ვერ მოიძებნა
აირჩიეთ სამუშაო მაგიდაზე დასამატებელი პროგრამა
დისკების ხატულები თავისუფლად გადაადგილეთ სამუშაო მაგიდაზე ან დაალაგეთ აქ
დისკების ხატულების ნაგულისხმევი პარამეტრები აღდგა; დააჭირეთ გამოყენებას
ძიებას არცერთი პროგრამა არ ემთხვევა
PuppyPin-ის პარამეტრების საქაღალდის შექმნა ვერ მოხერხდა
PuppyPin-ის შექმნა ვერ მოხერხდა
ეს პროგრამა უკვე სამუშაო მაგიდაზეა
PuppyPin არასწორია: </pinboard> ვერ მოიძებნა
PuppyPin-ის განახლებისთვის მეხსიერება საკმარისი არ არის
PuppyPin-ის ჩაწერა ვერ მოხერხდა
დაემატა სამუშაო მაგიდას. გადაადგილებისას EssoraWM ახალ მდებარეობას შეინახავს
ეს პროგრამა სამუშაო მაგიდაზე არ არის
წაიშალა სამუშაო მაგიდიდან
დისკების ხატულების პარამეტრების შენახვა ვერ მოხერხდა
დისკების ხატულების პარამეტრები გამოყენებულია
დამალული დისკები კვლავ ხილულია
დისკების ხატულების პარამეტრები შეიცვალა; დააჭირეთ გამოყენებას
პროგრამები და ჩაშენებული დისკების ხატულები
პროგრამები
დისკების ხატულები
ძიება
აკრიფეთ პროგრამების გასაფილტრად
დამატება
წაშლა
დახურვა
ხილვადობა
დისკების ხატულების ჩვენება
შიდა დისკები
მოსახსნელი დისკები
ქსელური დისკები
წარწერების ჩვენება
წარწერის ჩარჩო
შებრუნებული რიგი
თითოეული დისკის ხატულა შეგიძლიათ თავისუფლად გადაათრიოთ სამუშაო მაგიდაზე.
გამოყენება ყველა დისკს არჩეული განლაგებით დაალაგებს.
განლაგება
ჰორიზონტალური
ვერტიკალური
ჰორიზონტალური მდებარეობა
მარცხნივ
ცენტრში
მარჯვნივ
ვერტიკალური მდებარეობა
ზემოთ
ქვემოთ
ხატულის ზომა
ჰორიზონტალური დაშორება
ვერტიკალური დაშორება
ჰორიზონტალური წანაცვლება
ვერტიკალური წანაცვლება
დამალული დისკების ჩვენება
გამოყენება
გამოყენებულია
ნაგულისხმევი
გახსნა
მოხსნა
მიერთება
სამუშაო მაგიდიდან წაშლა
ტომი %s
''')

if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    for lang, catalog in RAW.items():
        update_po(root / 'po' / f'{lang}.po', MESSAGES, catalog)
