# What is OTRP?
This is a modified version of simutrans that some features are added to.  
OTRP means Oneway Twoway Road Patch. The simutrans standard uses only one lane when you build highways that has two lanes for each direction. The most important aim of this project is to take full advantage of the two lanes.  

The thread in Simutrans International Forum: https://forum.simutrans.com/index.php?topic=16659.0  

As of version 14_5, OTRP is based on simutrans standard nightly r8541.

# Download
In addition to the executable binary, the ribi-arrow pak is required. Please download it from https://drive.google.com/open?id=0B_rSte9xAhLDanhta1ZsSVcwdzg and put it in your pakset.
You can download the OTRP executable binary from the links below. **(2018 July 8th, updated to ver 14_5.)**  
windows: https://drive.google.com/open?id=1-q9JR_7DjYPT22HLeANOWsNFwM9nz0vb  
mac: https://drive.google.com/open?id=1BIasM0oZptCnZTY2gm3KhC1jqPuxNk2G  
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
## overtaking mode
![fig1](images/fig1.png)  
Select a road icon **with holding a Ctrl key**, then you can select the overtaking mode.
- oneway:
- halt mode:
- twoway: Vehicles behaves as in simutrans standard.
- only loading convoy: Vehicles overtake only a stopping vehicle.
- prohibited: All overtaking is prohibited.
- inverted: Lane is reversed.

## lane affinity signs
As the amount of traffic increases, traffic jam at a junction like the image below can be often seen.  
![fig3](images/fig3.png)  

## etc  
- The income/cost message can be turned off in the display settings  or by assigning a key to simple_tool[38].

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
