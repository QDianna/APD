# tema1

Tema 1 APD
Ana Elena-Diana, 331CB

In aceasta tema am realizat implementarea in paralel a algoritmului Marching Squares,
care presupune o metoda de a delimita contururile dintr-o imagine. Am folosit
biblioteca Pthreads din C++ pentru a creaa threadurile si bariera folosita pentru
a sincroniza aceste threaduri.

-- functia main --
In main citesc imaginea si, daca este prea mare, o scalez la 2048x2048 (pentru uniformitate),
aloc memorie pentru 'new_image'. Aloc memorie pentru datele necesare: 'contour_map' si 'grid',
apoi creez thread-urile.
Dupa ce thread-urile mor, functia main afiseaza rezultatul prin functia 'write_ppm' - step 4 si
elibereaza resursele - step 5.2.

-- structura threadData --
Am folosit aceasta structura pentru a putea trimite mai multe date ca argument pentru
functia thread-urilor 'processImageSection'. Aceasta contine o referinta catre 'barrier',
pentru a putea sincroniza threadurile, referinte catre 'image', 'new_image', 'grid' si
'contour_map' pentru a putea scala imaginea si pentru a aplica algoritmul,
cat si alte variabile necesare.

-- functia de thread --
Aceasta functie atribuie fiecarui thread o parte din urmatoarele sarcini:
- initializarea 'contour_map' - step 0
- scalarea imaginii initiale - step 1
- sampling pe grid - step 2
- aplicarea alg marching squares - step 3
- eliberarea resurselor - step 5.1
Fiecare thread isi calculeaza portiunea din controur_map/imagine/resurse pe care urmeaza
sa o proceseze in functie de id-ul sau si de cate thread-uri au fost create si apoi aplica
pasii enumerati anterior. Concret, am paralelizat for-urile (exterioare) din functiile:
'init_contour_map', 'sample_grid', 'march' si 'free_resources' ale variantei
secventiale.
Sincronizarea consta in faptul ca dupa fiecare pas, thread-urile
vor fi blocate intr-o bariera pana ce fiecare dintre ele proceseaza portiunea lor
de imagine, deoarece fiecare noua etapa foloseste elemente calculate in etapele
anterioare.

