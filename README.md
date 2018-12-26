# WemosRfidMPD
How to control playlists on MPD using an Wemos D1 8266 and an RC522 RFID reader

# Description
I recenlty bought some hardware and sensors to tinker with. This time it was the combination of a Wemos D1 mini Pro (16MByte) and 
a RFID Reader (RC522). Goal? First, to get everything working. Then? To find a practical use for it. After some looking around at 
my desk at home, I found that there was (yet again) a good use for this in controlling my *MPD* (MusicPlayerDaemon, 
see references at the end of this page), running
on RaspberryPi 1B. This time I decided to control playlists using the RFID-cards and -keychains. 

The idea is that for each different UID/RFID/card, there will be a different playlist. When switching cards it should first
save the current playlist (under the name of the RFID-UID found on the card), and then retrieve and load the playlist 
belonging to the new card.



## References
  * [Music Player Daemon](https://www.musicpd.org/)
  
