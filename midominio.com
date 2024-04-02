; Archivo de zona para midominio.com
$TTL 86400     ; Tiempo de vida (TTL) predeterminado de 1 día
$ORIGIN midominio.com.   ; Dominio base

; Registro de inicio de autoridad (SOA)
@   IN  SOA ns1.midominio.com. hostmaster.midominio.com. (
                    2023042101  ; Número de serie
                    3600        ; Actualización cada 1 hora
                    600         ; Reintento cada 10 minutos
                    604800      ; Expira en 1 semana
                    3600        ; Tiempo de vida negativo de 1 hora
                    )

; Registros de servidores de nombres (NS)
    IN  NS  ns1.midominio.com.
    IN  NS  ns2.midominio.com.

; Registros de direcciones IPv4 (A)
ns1     IN  A   192.168.1.1
ns2     IN  A   192.168.1.2
www     IN  A   192.168.1.3
mail    IN  A   192.168.1.4

; Registros de nombres canónicos (CNAME)
ftp     IN  CNAME   www.midominio.com.

; Registros de intercambio de correo (MX)
        IN  MX  10  mail.midominio.com.
        IN  MX  20  mail2.midominio.com.
mail2   IN  A   192.168.1.5