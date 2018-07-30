# What is OTRP?
This is a modified version of simutrans that some features are added to.  
OTRP means Oneway Twoway Road Patch. The simutrans standard uses only one lane when you build highways that has two lanes for each direction. The most important aim of this project is to take full advantage of the two lanes.  

The thread in Simutrans International Forum: https://forum.simutrans.com/index.php?topic=16659.0  

As of version 17, OTRP is based on simutrans standard nightly r8549.

# Download
In addition to the executable binary, the ribi-arrow pak is required. Please download it from https://drive.google.com/open?id=0B_rSte9xAhLDanhta1ZsSVcwdzg and put it in your pakset.
You can download the OTRP executable binary from the links below. **(2018 July 30th, updated to ver 17.)**  
windows: https://drive.google.com/open?id=14ZZ85PrcmUIITwBakbZj6Vx_dcUTIrdb  
mac: https://drive.google.com/open?id=1vBdji8ztr_kXu6H2MyvppqsCkVMPJypz  
source code: https://github.com/teamhimeh/simutrans/tree/OTRP-distribute  
OTRP does not provide specialized makeobj. Please use one of simutrans standard.

# Install
1. Download ribi-arrow and put it into your pakset.
1. In menuconf.tab in the pakset folder, assign an appropriate key to simple_tool[37]. For example, in menuconf.tab, add `simple_tool[37]=,:` and you can use RibiArrow by using the colon key.
1. Download the OTRP executable file and put it in the directory where simutrans.exe already exists.
1. Execute the file that you downloaded.
Please make sure not to overwrite your sve file of simutrans standard.

# How to use
Sorry, this section is still under construction.
## Road Configuration
![fig1](images/fig1.png)  
Select a road icon **with holding a Ctrl key**, then you can configure the road.

[overtaking mode]
- oneway: Road is oneway and vehicles can overtake others under the relaxed condition.
- halt mode: The behavior is same as oneway, except that the vehicle can load passenger on passing lane.
- twoway: Vehicles behaves as in simutrans standard.
- only loading convoy: Vehicles overtake only a stopping vehicle.
- prohibited: All overtaking is prohibited.
- inverted: Lane is reversed.
----
[others]
- avoid becoming cityroad: When this option is enabled, the road never becomes a cityroad.
- citycars do not enter: Citycars are prohibited to enter this road.

### Road configuration check tool
With holding the colon key, connected directions of roads can be checked with the arrow. This feature is available also through the display settings. If some options are enabled on the road, the color changes as follows:
- avoid becoming citycar enabled -> Green
- citycars do not enter enabled -> Magenda
- Both are enabled -> Orange

## lane affinity signs
As the amount of traffic increases, traffic jam at a junction like the image below can be often seen.  
![fig3](images/fig3.png)  

## etc  
- The income/cost message can be turned off in the display settings  or by assigning a key to simple_tool[38].
- To realize smooth traffic in an intersection, vehicles reserve tiles in the intersection. With b key, you can check and cancel the reservation of road.
- Use "make halt public" tool with the shift key, and the halt belongs to the current activated player. With the control key, this operation can be done with no cost.
- If you use the land raise/lower tool with ctrl key, the height of selected area are set to the height of the original coordinate. (Implemented by shingoushori)

# Parameters of OTRP
Most of these are stored in the game.
- **citycar_max_look_forward** : define the distance for which citycars determine their route.
- **citycar_route_weight_crowded**, **citycar_route_weight_vacant**, **citycar_route_weight_speed** : parameters for calculating citycars' route.

# Compatibility
## Compatibility of Add-ons
You can use all add-ons for simutrans standard on OTRP. There is no OTRP-specialized add-ons.
## Compatibility of the save data
- Save data of simutrans standard can be loaded on OTRP.
- **When you load the data of OTRP v12 or v13, please press the button "This is a data of OTRP v12 or v13." before the loading.**  
- When you launch OTRP v14 **for the first time**, please **delete autosave.sve** to prevent a crash.
- No compatibility for ex-OTRP.
- If you press "Readable by standard" button, the game is saved so that it is readable by simutrans standard. In this mode, please be ware that **OTRP specific data is lost.**

# Contact Me
If you have any questions, please feel free to ask me through the simutrans international forum or @himeshi_hob in Twitter. Every issue reports and your opinions are greatly appreciated.
