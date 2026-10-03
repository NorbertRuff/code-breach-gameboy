<?xml version="1.0" encoding="UTF-8"?>
<tileset version="1.10" tiledversion="1.12.2" name="greybox" tilewidth="8" tileheight="8" tilecount="8" columns="8">
 <image source="greybox.png" width="64" height="8"/>
 <tile id="0">
  <properties>
   <property name="name" value="empty"/>
  </properties>
 </tile>
 <tile id="1">
  <properties>
   <property name="name" value="solid"/>
   <property name="collision" value="solid"/>
  </properties>
 </tile>
 <tile id="2">
  <properties>
   <property name="name" value="one_way"/>
   <property name="collision" value="one_way"/>
  </properties>
 </tile>
 <tile id="3">
  <properties>
   <property name="name" value="hazard"/>
   <property name="collision" value="hazard"/>
  </properties>
 </tile>
 <tile id="4" type="gate">
  <properties>
   <property name="name" value="gate"/>
  </properties>
 </tile>
 <tile id="5" type="exit">
  <properties>
   <property name="name" value="exit"/>
  </properties>
 </tile>
 <tile id="6" type="fragment">
  <properties>
   <property name="name" value="fragment"/>
  </properties>
 </tile>
 <tile id="7" type="player_start">
  <properties>
   <property name="name" value="player_start"/>
  </properties>
 </tile>
</tileset>
