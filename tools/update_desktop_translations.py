#!/usr/bin/env python3
from pathlib import Path
from po_tools import update_po, update_pot

MESSAGES = [
'Could not open the X display',
'EssoraWM Desktop Manager',
'No application launchers were found',
'Select an application to add it to the desktop',
'Move drive icons freely on the desktop or arrange them here',
'Default drive icon settings restored; press Apply',
'No applications match the search',
'Could not create the PuppyPin configuration directory',
'Could not create PuppyPin',
'This application is already on the desktop',
'PuppyPin is invalid: </pinboard> was not found',
'Not enough memory to update PuppyPin',
'Could not write PuppyPin',
'Added to the desktop. EssoraWM will save the new position when you move it',
'This application is not on the desktop',
'Removed from the desktop',
'Could not save drive icon settings',
'Drive icon settings applied',
'Hidden drives are visible again',
'Drive icon settings changed; press Apply',
'Applications and native drive icons',
'Applications',
'Drive icons',
'Search',
'Type to filter applications',
'Add',
'Remove',
'Close',
'Visibility',
'Show drive icons',
'Internal drives',
'Removable drives',
'Network drives',
'Show labels',
'Label frame',
'Reverse order',
'You can also drag each drive icon freely on the desktop.',
'Apply arranges all drives using the selected layout.',
'Layout',
'Horizontal',
'Vertical',
'Horizontal position',
'Left',
'Center',
'Right',
'Vertical position',
'Top',
'Bottom',
'Icon size',
'Horizontal spacing',
'Vertical spacing',
'Horizontal offset',
'Vertical offset',
'Show hidden drives',
'Apply',
'Applied',
'Defaults',
'Open',
'Unmount',
'Mount',
'Remove from desktop',
'Volume %s',
]

RAW = {}

def add(lang, text):
    values = [line.strip() for line in text.strip().splitlines()]
    if len(values) != len(MESSAGES):
        raise SystemExit(f'{lang}: expected {len(MESSAGES)} translations, got {len(values)}')
    RAW[lang] = dict(zip(MESSAGES, values))

add('es', r'''
No se pudo abrir la pantalla X
Administrador del escritorio de EssoraWM
No se encontraron lanzadores de aplicaciones
Seleccione una aplicación para agregarla al escritorio
Mueva libremente los iconos de unidades en el escritorio u ordénelos aquí
Se restauró la configuración predeterminada de los iconos de unidades; pulse Aplicar
Ninguna aplicación coincide con la búsqueda
No se pudo crear el directorio de configuración de PuppyPin
No se pudo crear PuppyPin
Esta aplicación ya está en el escritorio
PuppyPin no es válido: no se encontró </pinboard>
No hay memoria suficiente para actualizar PuppyPin
No se pudo escribir PuppyPin
Agregada al escritorio. EssoraWM guardará la nueva posición cuando la mueva
Esta aplicación no está en el escritorio
Eliminada del escritorio
No se pudo guardar la configuración de los iconos de unidades
Configuración de iconos de unidades aplicada
Las unidades ocultas vuelven a estar visibles
La configuración de los iconos de unidades cambió; pulse Aplicar
Aplicaciones e iconos de unidades nativos
Aplicaciones
Iconos de unidades
Buscar
Escriba para filtrar aplicaciones
Agregar
Quitar
Cerrar
Visibilidad
Mostrar iconos de unidades
Unidades internas
Unidades extraíbles
Unidades de red
Mostrar etiquetas
Marco de etiquetas
Invertir orden
También puede arrastrar libremente cada icono de unidad por el escritorio.
Aplicar ordena todas las unidades con la disposición seleccionada.
Disposición
Horizontal
Vertical
Posición horizontal
Izquierda
Centro
Derecha
Posición vertical
Arriba
Abajo
Tamaño de icono
Espaciado horizontal
Espaciado vertical
Desplazamiento horizontal
Desplazamiento vertical
Mostrar unidades ocultas
Aplicar
Aplicado
Valores predeterminados
Abrir
Desmontar
Montar
Quitar del escritorio
Volumen %s
''')

add('de', r'''
Die X-Anzeige konnte nicht geöffnet werden
EssoraWM-Desktopverwaltung
Keine Anwendungsstarter gefunden
Wählen Sie eine Anwendung aus, die zum Desktop hinzugefügt werden soll
Laufwerkssymbole frei auf dem Desktop verschieben oder hier anordnen
Standardeinstellungen der Laufwerkssymbole wiederhergestellt; auf Anwenden klicken
Keine Anwendung entspricht der Suche
Das PuppyPin-Konfigurationsverzeichnis konnte nicht erstellt werden
PuppyPin konnte nicht erstellt werden
Diese Anwendung befindet sich bereits auf dem Desktop
PuppyPin ist ungültig: </pinboard> wurde nicht gefunden
Nicht genügend Speicher zum Aktualisieren von PuppyPin
PuppyPin konnte nicht geschrieben werden
Zum Desktop hinzugefügt. EssoraWM speichert die neue Position beim Verschieben
Diese Anwendung befindet sich nicht auf dem Desktop
Vom Desktop entfernt
Die Einstellungen der Laufwerkssymbole konnten nicht gespeichert werden
Einstellungen der Laufwerkssymbole angewendet
Ausgeblendete Laufwerke sind wieder sichtbar
Einstellungen der Laufwerkssymbole geändert; auf Anwenden klicken
Anwendungen und native Laufwerkssymbole
Anwendungen
Laufwerkssymbole
Suchen
Tippen, um Anwendungen zu filtern
Hinzufügen
Entfernen
Schließen
Sichtbarkeit
Laufwerkssymbole anzeigen
Interne Laufwerke
Wechseldatenträger
Netzlaufwerke
Beschriftungen anzeigen
Beschriftungsrahmen
Reihenfolge umkehren
Jedes Laufwerkssymbol kann auch frei auf dem Desktop verschoben werden.
Anwenden ordnet alle Laufwerke nach dem ausgewählten Layout an.
Layout
Horizontal
Vertikal
Horizontale Position
Links
Mitte
Rechts
Vertikale Position
Oben
Unten
Symbolgröße
Horizontaler Abstand
Vertikaler Abstand
Horizontaler Versatz
Vertikaler Versatz
Ausgeblendete Laufwerke anzeigen
Anwenden
Angewendet
Standardwerte
Öffnen
Aushängen
Einhängen
Vom Desktop entfernen
Datenträger %s
''')

add('fr', r'''
Impossible d’ouvrir l’affichage X
Gestionnaire du bureau EssoraWM
Aucun lanceur d’application trouvé
Sélectionnez une application à ajouter au bureau
Déplacez librement les icônes de lecteurs sur le bureau ou organisez-les ici
Paramètres par défaut des icônes de lecteurs restaurés ; cliquez sur Appliquer
Aucune application ne correspond à la recherche
Impossible de créer le dossier de configuration de PuppyPin
Impossible de créer PuppyPin
Cette application se trouve déjà sur le bureau
PuppyPin n’est pas valide : </pinboard> est introuvable
Mémoire insuffisante pour mettre à jour PuppyPin
Impossible d’écrire PuppyPin
Ajoutée au bureau. EssoraWM enregistrera la nouvelle position lors du déplacement
Cette application ne se trouve pas sur le bureau
Supprimée du bureau
Impossible d’enregistrer les paramètres des icônes de lecteurs
Paramètres des icônes de lecteurs appliqués
Les lecteurs masqués sont de nouveau visibles
Les paramètres des icônes de lecteurs ont changé ; cliquez sur Appliquer
Applications et icônes de lecteurs natives
Applications
Icônes de lecteurs
Rechercher
Saisissez du texte pour filtrer les applications
Ajouter
Supprimer
Fermer
Visibilité
Afficher les icônes de lecteurs
Lecteurs internes
Lecteurs amovibles
Lecteurs réseau
Afficher les étiquettes
Cadre des étiquettes
Inverser l’ordre
Vous pouvez aussi déplacer librement chaque icône de lecteur sur le bureau.
Appliquer organise tous les lecteurs selon la disposition sélectionnée.
Disposition
Horizontale
Verticale
Position horizontale
Gauche
Centre
Droite
Position verticale
Haut
Bas
Taille des icônes
Espacement horizontal
Espacement vertical
Décalage horizontal
Décalage vertical
Afficher les lecteurs masqués
Appliquer
Appliqué
Valeurs par défaut
Ouvrir
Démonter
Monter
Supprimer du bureau
Volume %s
''')

add('it', r'''
Impossibile aprire lo schermo X
Gestore del desktop EssoraWM
Nessun lanciatore di applicazioni trovato
Seleziona un’applicazione da aggiungere al desktop
Sposta liberamente le icone delle unità sul desktop oppure disponile qui
Impostazioni predefinite delle icone delle unità ripristinate; premi Applica
Nessuna applicazione corrisponde alla ricerca
Impossibile creare la cartella di configurazione di PuppyPin
Impossibile creare PuppyPin
Questa applicazione è già sul desktop
PuppyPin non è valido: </pinboard> non è stato trovato
Memoria insufficiente per aggiornare PuppyPin
Impossibile scrivere PuppyPin
Aggiunta al desktop. EssoraWM salverà la nuova posizione quando la sposti
Questa applicazione non è sul desktop
Rimossa dal desktop
Impossibile salvare le impostazioni delle icone delle unità
Impostazioni delle icone delle unità applicate
Le unità nascoste sono di nuovo visibili
Le impostazioni delle icone delle unità sono cambiate; premi Applica
Applicazioni e icone native delle unità
Applicazioni
Icone delle unità
Cerca
Digita per filtrare le applicazioni
Aggiungi
Rimuovi
Chiudi
Visibilità
Mostra icone delle unità
Unità interne
Unità rimovibili
Unità di rete
Mostra etichette
Cornice delle etichette
Inverti ordine
Puoi anche trascinare liberamente ogni icona di unità sul desktop.
Applica dispone tutte le unità secondo il layout selezionato.
Disposizione
Orizzontale
Verticale
Posizione orizzontale
Sinistra
Centro
Destra
Posizione verticale
In alto
In basso
Dimensione icona
Spaziatura orizzontale
Spaziatura verticale
Spostamento orizzontale
Spostamento verticale
Mostra unità nascoste
Applica
Applicato
Predefiniti
Apri
Smonta
Monta
Rimuovi dal desktop
Volume %s
''')

add('pt', r'''
Não foi possível abrir o ecrã X
Gestor do ambiente de trabalho EssoraWM
Não foram encontrados lançadores de aplicações
Selecione uma aplicação para adicionar ao ambiente de trabalho
Mova livremente os ícones das unidades no ambiente de trabalho ou organize-os aqui
Definições padrão dos ícones das unidades restauradas; prima Aplicar
Nenhuma aplicação corresponde à pesquisa
Não foi possível criar a pasta de configuração do PuppyPin
Não foi possível criar o PuppyPin
Esta aplicação já está no ambiente de trabalho
O PuppyPin é inválido: </pinboard> não foi encontrado
Memória insuficiente para atualizar o PuppyPin
Não foi possível escrever o PuppyPin
Adicionada ao ambiente de trabalho. O EssoraWM guardará a nova posição ao movê-la
Esta aplicação não está no ambiente de trabalho
Removida do ambiente de trabalho
Não foi possível guardar as definições dos ícones das unidades
Definições dos ícones das unidades aplicadas
As unidades ocultas estão novamente visíveis
As definições dos ícones das unidades foram alteradas; prima Aplicar
Aplicações e ícones nativos de unidades
Aplicações
Ícones de unidades
Pesquisar
Escreva para filtrar aplicações
Adicionar
Remover
Fechar
Visibilidade
Mostrar ícones de unidades
Unidades internas
Unidades amovíveis
Unidades de rede
Mostrar etiquetas
Moldura das etiquetas
Inverter ordem
Também pode arrastar livremente cada ícone de unidade no ambiente de trabalho.
Aplicar organiza todas as unidades com a disposição selecionada.
Disposição
Horizontal
Vertical
Posição horizontal
Esquerda
Centro
Direita
Posição vertical
Topo
Fundo
Tamanho do ícone
Espaçamento horizontal
Espaçamento vertical
Deslocamento horizontal
Deslocamento vertical
Mostrar unidades ocultas
Aplicar
Aplicado
Predefinições
Abrir
Desmontar
Montar
Remover do ambiente de trabalho
Volume %s
''')

add('hu', r'''
Az X kijelző nem nyitható meg
EssoraWM asztalkezelő
Nem találhatók alkalmazásindítók
Válasszon egy alkalmazást az asztalhoz adáshoz
A meghajtóikonok szabadon mozgathatók az asztalon, vagy itt rendezhetők
Az alapértelmezett meghajtóikon-beállítások visszaállítva; nyomja meg az Alkalmaz gombot
Nincs a keresésnek megfelelő alkalmazás
A PuppyPin beállítási könyvtára nem hozható létre
A PuppyPin nem hozható létre
Ez az alkalmazás már az asztalon van
A PuppyPin érvénytelen: a </pinboard> nem található
Nincs elég memória a PuppyPin frissítéséhez
A PuppyPin nem írható
Hozzáadva az asztalhoz. Az EssoraWM elmenti az új helyet az ikon mozgatásakor
Ez az alkalmazás nincs az asztalon
Eltávolítva az asztalról
A meghajtóikonok beállításai nem menthetők
A meghajtóikonok beállításai alkalmazva
A rejtett meghajtók ismét láthatók
A meghajtóikonok beállításai megváltoztak; nyomja meg az Alkalmaz gombot
Alkalmazások és natív meghajtóikonok
Alkalmazások
Meghajtóikonok
Keresés
Gépeljen az alkalmazások szűréséhez
Hozzáadás
Eltávolítás
Bezárás
Láthatóság
Meghajtóikonok megjelenítése
Belső meghajtók
Cserélhető meghajtók
Hálózati meghajtók
Címkék megjelenítése
Címkekeret
Fordított sorrend
Az egyes meghajtóikonok szabadon is húzhatók az asztalon.
Az Alkalmaz a kiválasztott elrendezés szerint rendezi az összes meghajtót.
Elrendezés
Vízszintes
Függőleges
Vízszintes helyzet
Balra
Középre
Jobbra
Függőleges helyzet
Felül
Alul
Ikonméret
Vízszintes térköz
Függőleges térköz
Vízszintes eltolás
Függőleges eltolás
Rejtett meghajtók megjelenítése
Alkalmaz
Alkalmazva
Alapértékek
Megnyitás
Leválasztás
Csatolás
Eltávolítás az asztalról
%s kötet
''')

add('ru', r'''
Не удалось открыть экран X
Менеджер рабочего стола EssoraWM
Панели запуска приложений не найдены
Выберите приложение для добавления на рабочий стол
Свободно перемещайте значки дисков по рабочему столу или упорядочьте их здесь
Настройки значков дисков по умолчанию восстановлены; нажмите «Применить»
Нет приложений, соответствующих поиску
Не удалось создать каталог настроек PuppyPin
Не удалось создать PuppyPin
Это приложение уже находится на рабочем столе
PuppyPin недействителен: </pinboard> не найден
Недостаточно памяти для обновления PuppyPin
Не удалось записать PuppyPin
Добавлено на рабочий стол. EssoraWM сохранит новое положение при перемещении
Этого приложения нет на рабочем столе
Удалено с рабочего стола
Не удалось сохранить настройки значков дисков
Настройки значков дисков применены
Скрытые диски снова видимы
Настройки значков дисков изменены; нажмите «Применить»
Приложения и встроенные значки дисков
Приложения
Значки дисков
Поиск
Введите текст для фильтрации приложений
Добавить
Удалить
Закрыть
Видимость
Показывать значки дисков
Внутренние диски
Съёмные диски
Сетевые диски
Показывать подписи
Рамка подписи
Обратный порядок
Каждый значок диска также можно свободно перетаскивать по рабочему столу.
Кнопка «Применить» упорядочит все диски согласно выбранной схеме.
Расположение
Горизонтально
Вертикально
Положение по горизонтали
Слева
По центру
Справа
Положение по вертикали
Сверху
Снизу
Размер значка
Интервал по горизонтали
Интервал по вертикали
Смещение по горизонтали
Смещение по вертикали
Показать скрытые диски
Применить
Применено
По умолчанию
Открыть
Размонтировать
Смонтировать
Удалить с рабочего стола
Том %s
''')

add('ja', r'''
X ディスプレイを開けませんでした
EssoraWM デスクトップマネージャー
アプリケーションランチャーが見つかりません
デスクトップに追加するアプリケーションを選択してください
ドライブアイコンはデスクトップ上で自由に移動するか、ここで整列できます
ドライブアイコンの既定設定を復元しました。「適用」を押してください
検索に一致するアプリケーションはありません
PuppyPin の設定ディレクトリを作成できませんでした
PuppyPin を作成できませんでした
このアプリケーションはすでにデスクトップにあります
PuppyPin が無効です: </pinboard> が見つかりません
PuppyPin を更新するためのメモリが不足しています
PuppyPin に書き込めませんでした
デスクトップに追加しました。移動すると EssoraWM が新しい位置を保存します
このアプリケーションはデスクトップにありません
デスクトップから削除しました
ドライブアイコンの設定を保存できませんでした
ドライブアイコンの設定を適用しました
非表示のドライブが再び表示されました
ドライブアイコンの設定が変更されました。「適用」を押してください
アプリケーションとネイティブドライブアイコン
アプリケーション
ドライブアイコン
検索
入力してアプリケーションを絞り込む
追加
削除
閉じる
表示
ドライブアイコンを表示
内蔵ドライブ
リムーバブルドライブ
ネットワークドライブ
ラベルを表示
ラベル枠
逆順
各ドライブアイコンはデスクトップ上で自由にドラッグすることもできます。
「適用」を押すと、選択した配置ですべてのドライブを整列します。
配置
横
縦
水平位置
左
中央
右
垂直位置
上
下
アイコンサイズ
水平間隔
垂直間隔
水平オフセット
垂直オフセット
非表示のドライブを表示
適用
適用済み
既定値
開く
アンマウント
マウント
デスクトップから削除
ボリューム %s
''')

add('zh_CN', r'''
无法打开 X 显示器
EssoraWM 桌面管理器
未找到应用程序启动器
选择要添加到桌面的应用程序
可在桌面上自由移动驱动器图标，或在此排列
已恢复驱动器图标默认设置；请点击“应用”
没有与搜索匹配的应用程序
无法创建 PuppyPin 配置目录
无法创建 PuppyPin
此应用程序已在桌面上
PuppyPin 无效：未找到 </pinboard>
没有足够的内存更新 PuppyPin
无法写入 PuppyPin
已添加到桌面。移动时 EssoraWM 将保存新位置
此应用程序不在桌面上
已从桌面移除
无法保存驱动器图标设置
已应用驱动器图标设置
隐藏的驱动器已重新显示
驱动器图标设置已更改；请点击“应用”
应用程序和原生驱动器图标
应用程序
驱动器图标
搜索
输入内容以筛选应用程序
添加
移除
关闭
可见性
显示驱动器图标
内部驱动器
可移动驱动器
网络驱动器
显示标签
标签边框
反向排列
也可以在桌面上自由拖动每个驱动器图标。
“应用”会按所选布局排列所有驱动器。
布局
水平
垂直
水平位置
左侧
居中
右侧
垂直位置
顶部
底部
图标大小
水平间距
垂直间距
水平偏移
垂直偏移
显示隐藏驱动器
应用
已应用
默认值
打开
卸载
挂载
从桌面移除
卷 %s
''')

add('zh_TW', r'''
無法開啟 X 顯示器
EssoraWM 桌面管理員
找不到應用程式啟動器
選擇要加入桌面的應用程式
可在桌面上自由移動磁碟圖示，或在此排列
已還原磁碟圖示預設設定；請按「套用」
沒有符合搜尋的應用程式
無法建立 PuppyPin 設定目錄
無法建立 PuppyPin
此應用程式已在桌面上
PuppyPin 無效：找不到 </pinboard>
沒有足夠的記憶體更新 PuppyPin
無法寫入 PuppyPin
已加入桌面。移動時 EssoraWM 將儲存新位置
此應用程式不在桌面上
已從桌面移除
無法儲存磁碟圖示設定
已套用磁碟圖示設定
隱藏的磁碟已重新顯示
磁碟圖示設定已變更；請按「套用」
應用程式與原生磁碟圖示
應用程式
磁碟圖示
搜尋
輸入內容以篩選應用程式
加入
移除
關閉
可見性
顯示磁碟圖示
內部磁碟
可移除磁碟
網路磁碟
顯示標籤
標籤外框
反向排列
也可以在桌面上自由拖曳每個磁碟圖示。
「套用」會依選取的配置排列所有磁碟。
配置
水平
垂直
水平位置
左側
置中
右側
垂直位置
頂端
底端
圖示大小
水平間距
垂直間距
水平偏移
垂直偏移
顯示隱藏磁碟
套用
已套用
預設值
開啟
卸載
掛載
從桌面移除
磁碟區 %s
''')

add('ar', r'''
تعذر فتح شاشة X
مدير سطح المكتب EssoraWM
لم يتم العثور على مشغلات تطبيقات
اختر تطبيقًا لإضافته إلى سطح المكتب
حرّك أيقونات الأقراص بحرية على سطح المكتب أو رتّبها هنا
تمت استعادة الإعدادات الافتراضية لأيقونات الأقراص؛ اضغط تطبيق
لا توجد تطبيقات تطابق البحث
تعذر إنشاء مجلد إعداد PuppyPin
تعذر إنشاء PuppyPin
هذا التطبيق موجود بالفعل على سطح المكتب
ملف PuppyPin غير صالح: لم يتم العثور على </pinboard>
لا توجد ذاكرة كافية لتحديث PuppyPin
تعذرت الكتابة إلى PuppyPin
تمت الإضافة إلى سطح المكتب. سيحفظ EssoraWM الموضع الجديد عند نقلها
هذا التطبيق غير موجود على سطح المكتب
تمت الإزالة من سطح المكتب
تعذر حفظ إعدادات أيقونات الأقراص
تم تطبيق إعدادات أيقونات الأقراص
أصبحت الأقراص المخفية مرئية مرة أخرى
تغيرت إعدادات أيقونات الأقراص؛ اضغط تطبيق
التطبيقات وأيقونات الأقراص الأصلية
التطبيقات
أيقونات الأقراص
بحث
اكتب لتصفية التطبيقات
إضافة
إزالة
إغلاق
الظهور
إظهار أيقونات الأقراص
الأقراص الداخلية
الأقراص القابلة للإزالة
أقراص الشبكة
إظهار التسميات
إطار التسميات
عكس الترتيب
يمكنك أيضًا سحب كل أيقونة قرص بحرية على سطح المكتب.
يرتّب زر تطبيق جميع الأقراص وفق التخطيط المحدد.
التخطيط
أفقي
عمودي
الموضع الأفقي
يسار
وسط
يمين
الموضع العمودي
أعلى
أسفل
حجم الأيقونة
التباعد الأفقي
التباعد العمودي
الإزاحة الأفقية
الإزاحة العمودية
إظهار الأقراص المخفية
تطبيق
تم التطبيق
الإعدادات الافتراضية
فتح
إلغاء الضم
ضم
إزالة من سطح المكتب
وحدة التخزين %s
''')

add('ca', r'''
No s’ha pogut obrir la pantalla X
Gestor de l’escriptori EssoraWM
No s’han trobat llançadors d’aplicacions
Seleccioneu una aplicació per afegir-la a l’escriptori
Moveu lliurement les icones de les unitats per l’escriptori o ordeneu-les aquí
S’han restaurat els valors predeterminats de les icones d’unitats; premeu Aplica
Cap aplicació coincideix amb la cerca
No s’ha pogut crear la carpeta de configuració de PuppyPin
No s’ha pogut crear PuppyPin
Aquesta aplicació ja és a l’escriptori
PuppyPin no és vàlid: no s’ha trobat </pinboard>
No hi ha prou memòria per actualitzar PuppyPin
No s’ha pogut escriure PuppyPin
Afegida a l’escriptori. EssoraWM desarà la nova posició quan la moveu
Aquesta aplicació no és a l’escriptori
Eliminada de l’escriptori
No s’han pogut desar els paràmetres de les icones d’unitats
S’han aplicat els paràmetres de les icones d’unitats
Les unitats amagades tornen a ser visibles
Els paràmetres de les icones d’unitats han canviat; premeu Aplica
Aplicacions i icones d’unitats natives
Aplicacions
Icones d’unitats
Cerca
Escriviu per filtrar aplicacions
Afegeix
Elimina
Tanca
Visibilitat
Mostra les icones d’unitats
Unitats internes
Unitats extraïbles
Unitats de xarxa
Mostra les etiquetes
Marc de les etiquetes
Inverteix l’ordre
També podeu arrossegar lliurement cada icona d’unitat per l’escriptori.
Aplica ordena totes les unitats amb la disposició seleccionada.
Disposició
Horitzontal
Vertical
Posició horitzontal
Esquerra
Centre
Dreta
Posició vertical
Dalt
Baix
Mida de la icona
Espaiat horitzontal
Espaiat vertical
Desplaçament horitzontal
Desplaçament vertical
Mostra les unitats amagades
Aplica
Aplicat
Valors predeterminats
Obre
Desmunta
Munta
Elimina de l’escriptori
Volum %s
''')

# Brazilian Portuguese uses the same catalog with a few regional terms.
RAW['pt_BR'] = dict(RAW['pt'])
RAW['pt_BR'].update({
    'Could not open the X display': 'Não foi possível abrir a tela X',
    'EssoraWM Desktop Manager': 'Gerenciador da área de trabalho EssoraWM',
    'Select an application to add it to the desktop': 'Selecione um aplicativo para adicionar à área de trabalho',
    'Applications': 'Aplicativos',
    'Type to filter applications': 'Digite para filtrar aplicativos',
    'Removable drives': 'Unidades removíveis',
    'Defaults': 'Padrões',
    'Remove from desktop': 'Remover da área de trabalho',
})


def ensure_po_header(path: Path, lang: str):
    if path.exists():
        return
    plural = 'nplurals=2; plural=(n != 1);'
    if lang in {'ja', 'zh_CN', 'zh_TW'}:
        plural = 'nplurals=1; plural=0;'
    elif lang == 'ar':
        plural = 'nplurals=6; plural=(n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 && n%100<=99 ? 4 : 5);'
    path.write_text(
        '# EssoraWM desktop translations.\n'
        'msgid ""\nmsgstr ""\n'
        '"Project-Id-Version: EssoraWM 0.1.11\\n"\n'
        f'"Language: {lang}\\n"\n'
        '"MIME-Version: 1.0\\n"\n'
        '"Content-Type: text/plain; charset=UTF-8\\n"\n'
        '"Content-Transfer-Encoding: 8bit\\n"\n'
        f'"Plural-Forms: {plural}\\n"\n', encoding='utf-8')


def main():
    root = Path(__file__).resolve().parents[1]
    po_dir = root / 'po'
    update_pot(po_dir / 'jwm.pot', MESSAGES)
    for lang, catalog in RAW.items():
        po_path = po_dir / f'{lang}.po'
        ensure_po_header(po_path, lang)
        update_po(po_path, MESSAGES, catalog)
    linguas = [line.strip() for line in (po_dir / 'LINGUAS').read_text().splitlines() if line.strip()]
    for lang in ('ar', 'ca', 'ja'):
        if lang not in linguas:
            linguas.append(lang)
    (po_dir / 'LINGUAS').write_text('\n'.join(linguas) + '\n')

if __name__ == '__main__':
    main()
