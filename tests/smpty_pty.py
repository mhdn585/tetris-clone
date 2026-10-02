#!/usr/bin/env python3
"""Smoke test: ejecuta ./tetris en un pseudo-terminal, simula pulsaciones
y verifica que renderiza menu, ajustes y partida sin colapsar.
HOME se aiso en un directorio temporal: el test nunca toca los datos reales."""
import fcntl, os, pty, re, shutil, struct, sys, tempfile, termios, time

RUTA = os.path.dirname(os.path.abspath(__file__))
EJECUTABLE = os.path.join(RUTA, "..", "tetris")
HOME_PRUEBAS = tempfile.mkdtemp(prefix="tetris_pty_home_")

pid, fd = pty.fork()
if pid == 0:
    os.execve(EJECUTABLE, ["./tetris"], dict(os.environ, HOME=HOME_PRUEBAS))

# Tamano del pty: 63 filas x 168 columnas (tamanio de diseno del juego).
fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 63, 168, 0, 0))

salida = bytearray()

def drena(segundos):
    fin = time.time() + segundos
    while time.time() < fin:
        try:
            import select
            listo, _, _ = select.select([fd], [], [], 0.1)
            if listo:
                trozo = os.read(fd, 65536)
                if not trozo:
                    return False
                salida.extend(trozo)
        except OSError:
            return False
    return True

FLECHA_ARRIBA  = b"\x1b[A"
FLECHA_ABAJO   = b"\x1b[B"
FLECHA_DERECHA = b"\x1b[C"
FLECHA_IZQUIERDA = b"\x1b[D"

secuencias = [
    (0.8, None),               # fotogramas del menu
    (0.2, FLECHA_ABAJO),       # navegar el menu con flechas (no debe cerrar)
    (0.2, FLECHA_ARRIBA),
    (0.2, FLECHA_DERECHA),
    (0.3, None),
    (0.2, b"1"),               # jugar
    (1.5, None),               # caida natural
    (0.2, b"w"),               # rotar
    (0.2, b"a"),               # mover izquierda
    (0.2, b"d"),               # mover derecha
    (0.2, FLECHA_IZQUIERDA),   # mover con flechas
    (0.2, FLECHA_DERECHA),
    (0.2, FLECHA_ARRIBA),      # rotar con flecha
    (0.2, FLECHA_ABAJO),       # soft drop con flecha
    (0.6, b"s"),               # soft drop
    (2.5, None),               # bloqueos y lineas
    (0.2, b"p"),               # pausa
    (0.5, None),
    (0.2, b"p"),               # reanudar
    (0.5, None),
    (0.2, b"r"),               # reiniciar
    (1.0, None),
    (0.2, b"q"),               # al menu
    (0.5, None),
    (0.2, FLECHA_ABAJO),       # el menu sigue vivo tras volver de partida
    (0.3, None),
    (0.2, b"3"),               # entrar en ajustes
    (0.6, None),
    (0.2, b"2"),               # elegir la paleta MATRIX
    (0.6, None),
    (0.2, b"q"),               # volver al menu
    (0.4, None),
    (0.2, b"1"),               # jugar otra vez, ahora en verde fosforo
    (0.8, None),
    (0.2, b"q"),               # y de vuelta al menu
    (0.4, None),
    (0.2, b"q"),               # salir
    (0.5, None),
]

# Ventana de bytes crudos de la partida: desde que se elige JUGAR hasta que
# se pulsa Q para volver al menu.
inicio_partida = fin_partida = None

for espera, tecla in secuencias:
    if tecla:
        if tecla == b"1" and inicio_partida is None:
            inicio_partida = len(salida)
        if tecla == b"q" and inicio_partida is not None and fin_partida is None:
            fin_partida = len(salida)
        try:
            os.write(fd, tecla)
        except OSError:
            break
    if not drena(espera):
        break

try:
    pid_vivo, estado = os.waitpid(pid, os.WNOHANG)
    if pid_vivo == 0:
        os.kill(pid, 9)
        os.waitpid(pid, 0)
except ChildProcessError:
    pass

texto = salida.decode(errors="replace")
# Quita las secuencias ANSI: los colores intercalados romperian los literales.
limpio = re.sub(r"\033\[[0-9;?]*[A-Za-z]", "", texto)

# Ventana limpia de la partida (recorte en bytes CRUDOS, antes de despojar
# ANSI): debe arrancar cuando aparece el panel ("SIGUIENTE") y desde ese
# punto el menu ("JUGAR") no puede volver a verse.
if inicio_partida is not None and fin_partida is not None:
    ventana = bytes(salida[inicio_partida:fin_partida]).decode(errors="replace")
    partida_limpia = re.sub(r"\033\[[0-9;?]*[A-Za-z]", "", ventana)
else:
    partida_limpia = ""
ancla = partida_limpia.find("SIGUIENTE")
menu_borrado = ancla != -1 and "JUGAR" not in partida_limpia[ancla:]

SUELO       = "<!" + "=" * 20 + "!>"
ADORNO      = "  " + "\\/" * 10
FILA_VACIA  = "<!" + " ." * 10 + "!>"

comprobaciones = {
    "menu titulado":       "[][][] [][][] [][][] [][][] [][][] [][][]" in limpio,
    "opciones del menu":   "JUGAR" in limpio and "COMO SE JUEGA" in limpio and "AJUSTES" in limpio,
    "record en menu":      "RECORD" in limpio,
    "suelo del tablero":   SUELO in limpio,
    "adorno del tablero":  ADORNO in limpio,
    "puntos de fondo":     FILA_VACIA in limpio,
    "pausa visible":       "P A U S A" in limpio,
    "bloque coloreado":    "[]" in limpio,
    "panel puntos":        "PUNTOS" in limpio,
    "panel nivel":         "NIVEL" in limpio,
    "panel siguiente":     "SIGUIENTE" in limpio,
    "menu borrado al jugar": menu_borrado,
    "pantalla de ajustes": "A J U S T E S" in limpio and "PALETA" in limpio,
    "verde fosforo matrix": "38;2;0;255;65" in texto,
    "json de datos creado": os.path.exists(os.path.join(HOME_PRUEBAS, ".tetris_puntos.json")),
    "ayuda pintada":       "CONTROLES" in limpio or "COMO SE JUEGA" in limpio,
    "sin trazas de crash": "Segmentation" not in texto and "sanitizer" not in texto,
}

shutil.rmtree(HOME_PRUEBAS, ignore_errors=True)

ok = True
for nombre, paso in comprobaciones.items():
    print(("  [OK]    " if paso else "  [FALLA] ") + nombre)
    ok = ok and paso

sys.exit(0 if ok else 1)
