Jest to drugi projekt wykonany na kurs Systemy Operacyjne 2 do 
Cabaje. Jest to rozszerzenie pierwszego projektu (uruchamianie i poprawne kończenie wątków) o tak zwaną sekcję krytyczną, która jest konieczna w implementacji w przypadku wykorzystywania współdzielonych danych (uniknięcie zjawiska "undefined behaviour").
Program jest prostą  animacją, która przedstawia odbijające się kuleczki w terminalu, w którym po kilku sekundach inicjalizowana jest nowa i zaczyna się przesuwać w losowym z ośmiu kierunków, startując ze środka. W przypadku spotkania się dwóch kulek powstaje jedna (duża) kulka o wektorowej sumie prędkości tych dwóch zderzonych.
Dla łatwości organizacji i przedstawiania obiektów w dowolnym miejscu w 
terminalu Linuxowym wykorzystano bibliotekę ncurses, a sam projekt 
został napisany przy użyciu kompilatora g++ oraz edytora tekstu Sublime 
Text 3 Dev w języku programowania C++.
