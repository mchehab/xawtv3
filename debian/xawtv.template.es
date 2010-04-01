Template: xawtv/makedev
Type: boolean
Default: true
Description: Create video4linux (/dev/video*) special files?
Description-es: �Crear los ficheros especiales video4linux (/dev/video*)?

Template: xawtv/build-config
Type: boolean
Default: false
Description: Create a default configuration for xawtv?
 You can create a system-wide configuration file
 for xawtv with reasonable default values for the
 country you live in (which TV norm is used for
 example).
 .
 It is not required to have a global configuration
 file, but it will be more comfortable for your
 users if they find a working default configuration.
Description-es: �Crear una configuraci�n por defecto para xawtv?
 Puede crear un fichero de configuraci�n para xawtv general para su
 sistema con valores razonables para el pa�s en el que vive (el sistema
 de televisi�n que se utiliza por ejemplo).
 .
 No es obligatorio tener un fichero de configuraci�n global, pero ser�
 m�s c�modo para sus usuarios encontrarse una configuraci�n por defecto
 que funciona.

Template: xawtv/tvnorm
Type: select
Choices: PAL, SECAM, NTSC
Description: Which TV norm is used in your country?
Description-es: �Qu� sistema de televisi�n se usa en su pa�s?

Template: xawtv/freqtab
Type: select
Choices: us-bcast, us-cable, us-cable-hrc, japan-bcast, japan-cable, europe-west, europe-east, italy, newzealand, australia, ireland, france, china-bcast
Description: Which frequency table should be used?
 A frequency table is just a list of TV channel
 names/numbers and the corresponding broadcast
 frequencies for these channels.  Different regions
 use different standards here...
Description-es: �Que tabla de frecuencias se deber�a usar?
 Una tabla de frecuencias es s�lo una lista de nombres/n�meros de
 canales de televisi�n y sus correspondientes frecuencias de emisi�n.
 Diferentes regiones utilizan diferentes est�ndares para esto...

Template: xawtv/channel-scan
Type: boolean
Default: yes
Description: scan for TV stations?
 I can do a scan of all channels and put a list of
 the TV stations I've found into the config file.
 .
 This requires a working bttv driver.  If bttv isn't
 configured correctly I might not find the TV stations.
 .
 I'll try to pick up the channel names from videotext.
 This will work with PAL only.
Description-es: �Buscar cadenas de televisi�n?
 Puedo realizar una b�squeda de todos los canales y poner una lista de
 las cadenas de televisi�n encontradas en el fichero de configuraci�n.
 .
 Esto requiere un controlador bttv que funcione. Si bttv no se encuentra
 configurado correctamente puede que no encuentre las cadenas de
 televisi�n.
 .
 Tratar� de obtener los nombres de los canales del teletexto. Esto s�lo
 funcionar� con PAL.
