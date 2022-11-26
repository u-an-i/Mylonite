# Mylonite
3D hardware accelerated Qt geo-map view and methods

## Idea
replacement for KDE Marble within a Qt applicaton using that

### Background
KDE Marble is a provider of a (geo-)maps-mapped (2D and 3D) view and geo methods such as distance between 2 points on a globe (along the shortest distance above the globe=sphere surface) (if I remember correctly, if not i would try to align later).  
https://marble.kde.org

It is used in an aviation flight planning software, Little Navmap. Apparently KDE Marble is currently not nice to still integrate into that project.  
https://albar965.github.io/littlenavmap.html

A Standard which defines map data delivery across the internet exists. 1 service provider applying this Standard to 1 of its datasets was found by a web search.  
The Standards spec document is 129 pages of rather incomplete information about how data is supposed to be delivered and extracted from the delivery.  
The 1 service provider found is a "known entity" and also provides "classical" tiled satellite maps for consumption by http GET requests and accompanies good documentation.  
The Standard is https://www.ogc.org/standards/wmts  
The service provider is https://developer.tomtom.com/map-display-api/documentation/raster/satellite-tile (link to "classical" map)

## Target
zooming and panning 60 fps fluid.
