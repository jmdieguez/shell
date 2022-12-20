# Lab: shell

Repositorio para el esqueleto del [lab shell](https://fisop.github.io/7508/lab/shell) del curso Mendez-Fresia de **Sistemas Operativos (7508) - FIUBA**

El propósito de este lab es el de desarrollar la funcionalidad mínima que caracteriza a un intérprete de comandos shell similar a lo que realizan bash, zsh, fish.

https://fisop.github.io/7508/lab/shell/

### Búsqueda en $PATH

Un requisito básico en una shell es el poder ejecutar binarios. Esto comúnmente podríamos realizarlo con la syscall execve(2):

```
int execve(const char *pathname, char *const argv[],
                  char *const envp[]);
```
Al ejecutar la shell de la siguiente forma ```$ ./sh``` , estamos inicialmente privándonos de usar las variables de entorno y argumentos posibles para un programa, que pueden ser capturados en el mismo main:

```
int main(int argc, char *argv[], char *envp[])
```

Esto es lógico porque queremos imitar el comportamiento de la shell leyendo por entrada estándar las líneas que se deben correr, para su futuro parseo.
Además execve(2) no ejecuta binarios en $PATH, por lo que optamos por usar los wrappers de libc: exec(3).

Las llamadas de la librería con *p* en el nombre buscan la variable de entorno PATH para encontrar el programa si no tiene un directorio (es decir, no contiene un carácter /). Caso contrario, el nombre del programa se trata como una ruta al ejecutable.

```
	case EXEC: {
		e = (struct execcmd *) cmd;

		set_environ_vars(e->eargv, e->eargc);

		if (execvp(e->argv[0], e->argv) == ERROR) {
			fprintf_debug(stderr, "Error while exec \n");
			_exit(ERROR);
		}

		exit(0);
		break;
	}
  
```

Una vez ejecutado el comando, puede ser que la llamada a exec(3) falle, citando ```man exec``` :

  • Si se deniega el permiso para un archivo (el intento de execve(2) falló con el error EACCES), estas funciones
  continuarán buscando el resto de la ruta de búsqueda. Sin embargo, si no se encuentra ningún otro archivo, volverán
  con errno establecido en EACCES.

  • Si no se reconoce el encabezado de un archivo (el intento de ejecución (2) falló con el error ENOEXEC), estas funciones
  tions ejecutará el shell (/bin/sh) con la ruta del archivo como primer argumento. (Si este intento
  falla, no se realiza más búsqueda).

  • Las funciones exec(3) regresan solo si ha ocurrido un error. El valor de retorno es -1, y errno se establece conforme al error especificado por execve(2).
       
---

### Procesos en segundo plano

Se puede, si se desea, ejecutar procesos en segundo plano con el flag &. Al final de correr el comando, se esperan todos los procesos hijos que hayan terminado con ``` pid_t waitpid(pid_t pid, int *wstatus, int options); ``` Usando la opción WNOHANG, especificando que se retorne inmediatamente si ningún hijo termino.

```
int
run_cmd(char *cmd)
{

        ...
        
        	if (parsed->type == BACK)
		          print_back_info(parsed);  
        ...
        
        	while (waitpid(-1, NULL, WNOHANG) > 0);
          
       ...
}
```
---

### Flujo estándar

Nuestra shell soporta las siguientes redirecciones: >, <, 2> y 2>&1. Para entender la última redirección me gusta separarlo en dos partes:
 
 • 2> redirecciona la salida del fd = 2 (stderr) a:
 
 • & especifica que lo que viene después es un filedescriptor y no un nombre de archivo.
 • 1   fd.
 
 Se pueden combinar salidas, como por ejemplo: 
 ```
 
 (/home/manu)
$ ls -C /home /noexiste >out.txt 2>&1
        Program: [ls -C /home /noexiste >out.txt 2>&1] exited, status: 2
 ```
 
  • >out.txt redirecciona la salida estandar a el archivo out.txt.
  • 2>&1 redirecciona la salida de error estandar a el fd = 1 (stdout), que casualmente ahora es out.txt.

```
 (/home/manu)
$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
manu
```

Si vemos el contenido de out.txt, podemos ver que tenemos la salida estandar:
```
/home:
manu
```

Como también la de error:
```
ls: cannot access '/noexiste': No such file or directory
```

---

### Tuberías simples (pipes)

Al utilizar pipes, podemos acceder a la magic var para obtener el exit status del proceso. Esto, igual, solo funciona para el último proceso del pipe, ya que si este falla en el resto, no es posible conseguir el valor de retorno.

• bash

```
manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ ls -l | grep Doc | asdasd
asdasd: command not found
manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ echo $?
127

manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ ls -l | asdasd | wc
asdasd: command not found
      0       0       0
manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ echo $?
0

```

• ./sh

```
 (/home/manu)
$ ls -l | grep Doc | asdasd
Error while exec
 (/home/manu)
$ echo $?
65280
        Program: [echo $?] exited, status: 0
 (/home/manu)
$ ls -l | grep Doc | wc
      1       9      45
 (/home/manu)
$ echo $?
0
        Program: [echo $?] exited, status: 0
 (/home/manu)
$ ls -l | asdasd | wc
Error while exec
      0       0       0
 (/home/manu)
$ echo $?
0
```

---

### Variables de entorno adicionales

La shell tiene su propio entorno. Al hacer el fork del proceso a ejecutar, recién ahí establecemos las variables nuevas sobre el entorno heredado por la shell. 

---

### Pseudo-variables

• $- : Flags pasados a un script.
 ```
manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ echo 6 $-
6 himBHs
 ```
• $! : Pid del último proceso corrido en background.
 ```
manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ sleep 6 &
[1] 940
manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ echo 6
6
[1]+  Done                    sleep 6
manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ echo $!
940
 ```

• $_ : Último argumento.
 ```
manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ echo hola como estas
hola como estas
manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ echo $_
estas
```

---

### Comandos built-in

Al usar una shell, es importante la capacidad de moverse entre directorios, y al igual que salir cuando uno desea. Para esto, implementamos 3 funciones built-in comunes de bash:

  • cd - change directory (cambia el directorio actual).
  • exit - exits nicely (termina una terminal de forma linda).
  • pwd - print working directory (muestra el directorio actual de trabajo).
 
Si desde bash tratamos de buscar los binarios de cd y pwd, solo podemos encontrar al segundo.

 ```
manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ whereis cd
cd:

manu@manu:/mnt/c/Users/manuk/OneDrive/Documentos/sisop_2022b_dieguez/shell$ whereis pwd
pwd: /usr/bin/pwd /usr/include/pwd.h /usr/share/man/man1/pwd.1.gz

```

Esto nos dice que no hace falta una implementación built-in de tal comando, ya que podríamos simplemente ejecutarlo. Si bien esto es cierto, al tratarse de una operación tan básica como imprimir una línea, es válido tener nuestra propia versión y ahorrarnos buscar y ejecutar otros programas.

```
int
pwd(char *cmd)
{
	if (strcmp(cmd, "pwd") == 0) {
		char curdir[FNAMESIZE];
		printf("%s\n", getcwd(curdir, sizeof curdir));
		return PERFORMED;
	}
	return UNPERFORMED;
}
```

---

# Desafíos

## Enunciado

Implementar el comando built-in history, el mismo muestra la lista de comandos ejecutados hasta el momento. De proporcionarse como argumento n, un número entero, solamente mostrar los últimos n comandos.

De estar definida la variable de entorno HISTFILE, la misma contendrá la ruta al archivo con los comandos ejecutados en el pasado. En caso contrario, utilizar como ruta por omisión a ~/.fisop_history.

Permitir navegar el historial de comandos con las teclas ↑ y ↓, de modo tal que se pueda volver a ejecutar alguno de ellos. Con la tecla ↑ se accede a un comando anterior, y con la tecla ↓ a un comando posterior, si este último no existe, eliminar el comando actual de modo que solo se vea el prompt.

La tecla BackSpace debe funcionar para borrar los caracteres de un comando de hasta una línea de largo. Al presionar Ctrl + d, la shell debe terminar su ejecución.

## Resolución

La información importante para mantener registro del historial de procesos ejecutados decidió guardarse en las siguientes variables, las cuales son inicializadas con los datos persistidos en el archivo HISTFILE a elección. 

```
char *history[HISTORY_MAX_SIZE];
unsigned history_count = 0;
```

De los desafíos electivos, se decidió implementar los designadores de eventos **!!** y **!-n**, los cuales permiten ejecutar el último comando, y el n-ésimo más reciente, respectivamente. Cabe destacar que a partir de lo experimentado con los designadores tradicionales en bash, si el último comando en el historial es igual al que se introduce, este no se añade nuevamente al historial.

La shell soporta la navegación por historial y el borrado de caracteres a partir de los cambios efectuados en **readline.c**. Cabe destacar que fue
necesario el pasaje a modo no canónico, ya que al apretar las teclas ↑ , ↓ , y backspace, normalmente quedaría registrada en pantalla esa entrada.

VMIN y VTIME son macros que se utilizan como índices en la matriz termios.c_cc[], que en circunstancias normales contiene la lista de caracteres de control especiales (retroceso, borrado, eliminación de línea, interrupción, etc.) para la sesión actual, pero cuando el bit ICANON está desactivado, se selecciona un "modo sin procesar" que cambia la interpretación de estos valores.

VMIN es un recuento de caracteres que va de 0 a 255 caracteres, y VTIME es el tiempo medido en intervalos de 0,1 segundos (0 a 25,5 segundos).

Por ende, si VMIN > 0 y VTIME = 0:

Es una lectura contada que se satisface solo cuando al menos 1 caracter se ha transferido a partir del buffer; no hay ningún componente de tiempo involucrado.

### Salida esperada

```
manu@manu:~/Desktop/sisop_2022b_dieguez$ echo hola
hola
manu@manu:~/Desktop/sisop_2022b_dieguez$ echo chau
chau
manu@manu:~/Desktop/sisop_2022b_dieguez$ echo mundo
mundo
manu@manu:~/Desktop/sisop_2022b_dieguez$ history
    1  echo hola
    2  echo chau
    3  echo mundo
    4  history
manu@manu:~/Desktop/sisop_2022b_dieguez$ !-2
echo mundo
mundo
manu@manu:~/Desktop/sisop_2022b_dieguez$ !!
echo mundo
mundo
manu@manu:~/Desktop/sisop_2022b_dieguez$ history -c
manu@manu:~/Desktop/sisop_2022b_dieguez$ echo hola
hola
manu@manu:~/Desktop/sisop_2022b_dieguez$ echo mundo
mundo
manu@manu:~/Desktop/sisop_2022b_dieguez$ echo adios
adios
manu@manu:~/Desktop/sisop_2022b_dieguez$ ps
    PID TTY          TIME CMD
  20846 pts/0    00:00:00 bash
  24769 pts/0    00:00:00 ps
manu@manu:~/Desktop/sisop_2022b_dieguez$ pwd
/home/manu/Desktop/sisop_2022b_dieguez
manu@manu:~/Desktop/sisop_2022b_dieguez$ !-3
echo adios
adios
manu@manu:~/Desktop/sisop_2022b_dieguez$ history
    1  echo hola
    2  echo mundo
    3  echo adios
    4  ps
    5  pwd
    6  echo adios
    7  history
manu@manu:~/Desktop/sisop_2022b_dieguez$ echo fin
fin
manu@manu:~/Desktop/sisop_2022b_dieguez$ !!
echo fin
fin
manu@manu:~/Desktop/sisop_2022b_dieguez$ history 4
    6  echo adios
    7  history
    8  echo fin
    9  history 4
```

### Salida obtenida

```
 (/home/manu) 
$ echo hola
hola
	Program: [echo hola] exited, status: 0 
 (/home/manu) 
$ echo mundo
mundo
	Program: [echo mundo] exited, status: 0 
 (/home/manu) 
$ echo adios
adios
	Program: [echo adios] exited, status: 0 
 (/home/manu) 
$ ps
    PID TTY          TIME CMD
  19632 pts/1    00:00:00 bash
  24761 pts/1    00:00:00 sh
  24765 pts/1    00:00:00 ps
	Program: [ps] exited, status: 0 
 (/home/manu) 
$ pwd
/home/manu
 (/home/manu) 
$ !-3
adios
	Program: [echo adios] exited, status: 0 
 (/home/manu) 
$ history 
    1  echo hola
    2  echo mundo
    3  echo adios
    4  ps
    5  pwd
    6  echo adios
    7  history
 (/home/manu) 
$ echo fin
fin
	Program: [echo fin] exited, status: 0 
 (/home/manu) 
$ !!
fin
	Program: [echo fin] exited, status: 0 
 (/home/manu) 
$ history 4
    6  echo adios
    7  history
    8  echo fin
    9  history 4
   
```

Como se puede observar y probar, las salidas son prácticamente idénticas. Cabe aclarar que se partió desde un archivo de historial vacío (la shell lo carga cada vez que se inicia) y se eliminó el historial de bash con "history -c".

### Archivo de history

El archivo que persiste los datos relacionados al historial por defecto es .fisop_history, pero en caso de querer cambiarlo, es necesario primero definir su valor antes de hacer **make**:

		$ export HISTFILE=archivo
		$ make

Con esto se va a compilar el programa con el flag "-D HISTFILE", por lo que en tiempo de compilación se definirá el valor de la constante correspondiente al nombre del archivo.

```
manu@manu:~/Desktop/sisop_2022b_dieguez/shell$ cat ~/.fisop_history
echo hola
echo mundo
echo adios
ps
pwd
echo adios
history
echo fin
history 4
```

## Compilar y ejecutar

```bash
$ make run
```

## Linter

```bash
$ make format
```
