#pragma once 
inline const char* index_html = R"multiline( 
<!doctype html><html lang=en><meta charset=UTF-8><meta http-equiv=X-UA-Compatible content="IE=edge"><meta name=viewport content="width=device-width,initial-scale=1"><title>Solar Tracker</title><style>body{padding:0;margin:0}body *{box-sizing:border-box}#wsstate{height:2px;width:100%;background-color:red}#wsstate.on{background-color:green}</style><main><div id=wsstate></div><section id=advanced><h3>Preferances</h3><table id=prefs></table><button onclick=prefs_download()>Download</button> <button onclick=prefs_upload()>Upload</button> <button onclick=prefs_load()>Load</button> <button onclick=prefs_save()>Save</button><h3>Commands</h3><input id=cmd> <button onclick='cmd_execute(document.getElementById("cmd").value)'>Execute</button></section><section><h2>Witaj w Solar Trackerze! Żółć ęąśń</h2><fieldset><legend>Stan Osi Poziomej</legend><button onclick='cmd_execute("hmotor 0")'>Swobodny</button> <button onclick='cmd_execute("hmotor 1")'>Hamowanie Szybkie</button> <button onclick='cmd_execute("hmotor 2")'>Hamowanie</button> <button onclick='cmd_execute("hmotor 3")'>Ruch &lt;--</button> <button onclick='cmd_execute("hmotor 4")'>Ruch --&gt;</button> <button onclick='cmd_execute("hmotor 5")'>Kalibracja</button><br>Stan Aktualny: <span id=s_hst></span><br>Kalibracja: <span id=s_hcal></span><br>Pozycja: <span id=s_hpos></span> %<br>Czujnik Lewy: <span id=s_hs1></span> Omów<br>Czujnik Prawy: <span id=s_hs2></span> Omów</fieldset><button onclick='cmd_execute("auto")'>Włącz Tryb Automatyczny</button><br>Sterowanie Manualne: <span id=s_man></span><p></p><button onclick=toggle_advanced()>Opcje Zaawansowane</button></section></main><script>function decode_prefs(e){return e.split(",").map((e=>e.split("|")))}function encode_prefs(e){return e.map((e=>e.join("|"))).join(",")}function populate_prefs(e){prefs_elem.innerHTML=e.map((e=>"<tr>"+e.map((e=>'<td><input value="'+e+'"/></td>')).join("")+"</tr>")).join("")}function extract_prefs(){return Array.from(prefs_elem.rows).map((e=>Array.from(e.children).map((e=>e.firstChild.value)).join("|"))).join(",")}function decode_state(e){let s={},a=e.split("|");return s.manual="1"==a[1],s.hstate=a[2],s.hcal="1"==a[3],s.hmax=+a[4],s.hpos=+a[5],s.hsens1=+a[6],s.hsens2=+a[7],s}function populate_state(e){E("s_hst").innerHTML=e.hstate,E("s_hcal").innerHTML=e.hcal?"Tak":"Nie",E("s_hpos").innerHTML=e.hpos/e.hmax*100,E("s_hs1").innerHTML=e.hsens1,E("s_hs2").innerHTML=e.hsens2,E("s_man").innerHTML=e.manual?"Tak":"Nie"}function prefs_download(){ws.send("d")}function prefs_upload(){ws.send("u"+extract_prefs())}function cmd_execute(e){ws.send("c"+e)}function prefs_load(){cmd_execute("cfgload")}function prefs_save(){cmd_execute("cfgsave")}function request_state(){ws.send("r")}function toggle_advanced(){adv=1-adv,params.set("adv",adv),location.search=params}adv=1,params=new URLSearchParams(location.search),"1"!=params.get("adv")&&(document.getElementById("advanced").style.display="none",adv=0),prefs_elem=document.getElementById("prefs"),wsstate_elem=document.getElementById("wsstate"),wsurl=params.get("wsurl")||"ws://"+location.hostname+":81",ws=new WebSocket(wsurl),ws.onopen=e=>{console.log("WS opened"),wsstate_elem.className="on"},ws.onerror=e=>{console.log("WS error",e),wsstate_elem.className=""},ws.onclose="close",ws.onmessage=e=>{console.log("WS message",e.data),"d"==e.data[0]?populate_prefs(decode_prefs(e.data.slice(1))):"r"==e.data[0]&&(state=decode_state(e.data.slice(1)),populate_state(state))},E=e=>document.getElementById(e)</script>
)multiline"; 
