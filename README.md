# PBL-Mobile-Platform
Celem projektu jest budowa zdalnie sterowanej platformy, mogącej skanować otoczenie za pomocą lidara.

Gdzie platforma:

 - 12-14.11.2025 -> MR (testy kontrolera) (ok)
 - 15-17.11.2025 -> ML (testy odometrii + montaż enkoderów) (ok)
 - 18-19.11.2025 -> MR (testy kontrolera) (ok)
 - 19-25.11.2025 -> JG
 - 26-27.11.2025 -> ML
 - 28.11-17.12.2025 -> JG
 - 17.12-18.12.2025 -> MR
 - 19.12-08.01.2026 -> JG

TO DO + podział zadań:

* Platforma:
    0. Główna pętla
     - Software:
        - Integracja z kodem z receivera (partially)
        - Integracja z kodem z odometrii
        - Integracja z kodem do sterowania silniczkami
        - Integracja z kodem do lidara
        - Możliwość przesyłu danych na kompa (zrobić to jako osobną klasę zarządzającą danymi / zapis na microSD / przesył na kompa / kontroler.)

    1. System jezdny
     - Hardware:
        - Montaż enkoderów (ML) (ok)
     - Software:
        - Klasa kontrolująca silniczki (MŚ & JG)

    2. System pomiarowy:
     - Hardware:
        - Montaż Lidara (ML)
     - Software:
        - Test lidara na kompie (ML)
        - Test 1 klasy kontrolującej lidar (ML)

    3. System pozycyjny:
     - Hardware:
        - Enkodery z (1.) (ok)
     - Software:
        - Test 1 klasy z Odometrią (ML) (ok)

    4. Kontroler:
     - Hardware:
        - (ok)
     - Software:
        - Test przesyłu danych na platformę (MR) (OK)
        - Wyświetlacz e-ink (MR) (OK)

    5. System wykrywania przeszkód
     - Hardware:
        - Zaprojektowanie systemu (MR) (OK)
     - Software:
        - Hardware interrupt do podłączenia na platformę (MR) (OK)
        - Hardware interrupt platforma (ML) (test needed)
        - Asembler sczytujący wartości z pinów (MR) (OK)

* PC / Laptop:
    1. Aplikacja Desktopowa (wyświetla zebrane pomiary, przechowuje je w folderach):
        - Komunikacja z esp (https://l.messenger.com/l.php?u=https%3A%2F%2Fgithub.com%2FChuckMash%2FESPythoNOW&h=AT2EF5bl9oDyyULc6AOVDYfJnaDLg0rnqc17YCFMh-ZsuMV3QM-s7jL485ft-QLvyfkwyixq_B6JFsWAXf7_GO_xhrHctCMNtKN3DZeXKmGdo-a4BtHtPcib_21fGNs)

        - Przechowywanie skanów z pojedynczego skanu w pliku .pcd (https://pointclouds.org/documentation/tutorials/pcd_file_format.html)
        - Pliki .pcd w jednym folderze. Jeden folder na jedną sesję jeżdżenia roverem. 
        - (JG & MŚ)
        - Moduł LINUX (test)

* Zakupy:
    1. Zamówienie
        - Zapotrzebowanie 2 (MR) (ok)
        - Zapotrzebowanie 3 (MR) (ok)
    2. Odbiór 
        - (Kto będzie ten będzie) (ok)

* Odległa przyszłość (luźne pomysły jakby został kiedyś czas):
    1. Pełna wizualizacja 3D zebranych skanów
    2. SLAM ?
    3. Lidar obracany pod różnymi kątami, zbierający punkty z różnych płaszczyzn skanowania
    4. Pomiar nachylenia platformy / terenu
        - Dane same w sobie na temat terenu
        - 3D odometria (x, y, z)
    5. Algorytmy przetwarzania chmur punktów
        - Voxel Downsampling
        - Ball Pivoting 
        - Surface reconstruction
        - Poisson surface reconstruction
        - Plane fitting 
        - etc.

* Zasoby (biblioteki, schematy, takie tam):
    1. Komunikacja esp <-> komputer
        - (https://l.messenger.com/l.php?u=https%3A%2F%2Fgithub.com%2FChuckMash%2FESPythoNOW&h=AT2EF5bl9oDyyULc6AOVDYfJnaDLg0rnqc17YCFMh-ZsuMV3QM-s7jL485ft-QLvyfkwyixq_B6JFsWAXf7_GO_xhrHctCMNtKN3DZeXKmGdo-a4BtHtPcib_21fGNs)

    2. Format plików
        - https://pointclouds.org/documentation/tutorials/pcd_file_format.html

    3. Biblioteki do analizy chmur punktów
        - https://www.open3d.org/docs/latest/introduction.html
        - https://docs.opencv.org/4.12.0/

    4. Teoria
        - Odometria:
            - https://medium.com/@nahmed3536/wheel-odometry-model-for-differential-drive-robotics-91b85a012299

    4. Dokumentacje techniczne
        - Lidar
            - https://bucket-download.slamtec.com/d1e428e7efbdcd65a8ea111061794fb8d4ccd3a0/LD108_SLAMTEC_rplidar_datasheet_A1M8_v3.0_en.pdf
            - http://bucket.download.slamtec.com/351a5409ddfba077ad11ec5071e97ba5bf2c5d0a/LR002_SLAMTEC_rplidar_sdk_v1.0_en.pdf
        - L298N
            - https://botland.com.pl/sterowniki-silnikow-moduly/3164-l298n-dwukanalowy-sterownik-silnikow-modul-12v-2a-5904422359317.html 
        - HCSR-04
            - https://web.eece.maine.edu/~zhu/book/lab/HC-SR04%20User%20Manual.pdf
            - https://botland.com.pl/content/144-pomiar-odleglosci-z-wykorzystaniem-arduino-i-czujnika-hc-sr04-lub-us-015
        - Platforma:
            - https://botland.com.pl/podwozia-robotow/13744-chassis-round-2wd-2-kolowe-podwozie-robota-z-napedem-aluminiowe-szare-5904422342098.html
        
    5. Test change.

    6. Test na 2 kompie sry for that
    
