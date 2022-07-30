# ARM-KL05 LED náramkové hodiny na báze RTC modulu
Kinetis KL05 + LED display náramkové hodiny pomocou RTC modulu. Využitý modul LLWU a prechody do LLS. Implementované v Kinetis Design Studio.

## Zadanie
V Kinetis Design Studio (KDS) implementujte vstavanú aplikáciu v jazyku C pre mikrokontrolér Kinetis KL05, ktorá bude realizovať funkciu hodiniek s nasledujúcimi požiadavkami:
- informácie o čase budú zobrazované pomocou 7-segmentového LED displeja so štyrmi pozíciami pre jednotlivé číslice časového údaju
- k ovládaniu hodiniek (zobrazenie údajov o aktuálnom čase a jeho nastavovanie) bude slúžiť tlačítko dostupné na vývojovej doske
- čas bude meraný pomocou RTC periférie
- cieľom je vytvoriť funkčnú implementáciu hodiniek a súčasne dosiahnuť čo najnižšiu spotrebu s využitím vhodného režimu šetrenia (tzv. sleep mode), ktorých mikrokontrolér KL05 ponúka celú radu (LLS, VLLS, VLPS)
- projekt je možné realizovať bare-metal riešením alebo s využitím vhodného RTOS operačného systému (napr. FreeRTOS).

Ďaľšou zložkou pre hodnotenie je stručná dokumentácia poskytujúca základné informácie o projekte a jeho riešenia.
