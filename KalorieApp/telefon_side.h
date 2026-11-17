#pragma once

const char TELEFON_SIDE[] PROGMEM = R"html(
<!doctype html>
<html lang="da">
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>KalorieApp · Scan frugt</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#f5f4ef;color:#202a27;font:16px/1.5 system-ui,sans-serif}
main{max-width:440px;margin:32px auto;padding:24px}h1{font-size:28px;margin:0 0 8px}p{margin:12px 0}
input,button{font:inherit;width:100%;margin:10px 0;padding:12px;border:1px solid #cfd6ce;border-radius:10px}
button{background:#276448;color:white;cursor:pointer}button:disabled{opacity:.55;cursor:wait}
canvas{display:block;width:192px;height:192px;margin:20px auto;background:#e0e4dc;border-radius:12px}
.result{padding:16px;background:#fffefa;border:1px solid #dedfd7;border-radius:12px}.muted{font-size:14px;color:#58665d}
</style>
<main>
<h1>Scan din frugt</h1>
<p>Tag et billede af ét æble eller én banan. Kontrollér resultatet på ESP32-skærmen, før du accepterer.</p>
<input id="photo" type="file" accept="image/*" capture="environment" aria-label="Tag eller vælg et billede af frugten">
<canvas id="photo-preview" width="96" height="96" aria-label="Billedudsnittet, der sendes til modellen"></canvas>
<button id="scan" type="button">Scan frugt</button>
<section class="result" aria-live="polite"><strong id="result">Klar til scanning</strong><p id="status">Intet registreres, før du vælger “Accepter” på ESP32-skærmen.</p></section>
<p class="muted">Kalorierne beregnes ud fra en fast standardportion. Kameraet måler ikke frugtens vægt. Brug slideren til at vælge “Afvis” eller “Accepter”, og bekræft med et tryk.</p>
</main>
<script>
'use strict';
const photo=document.getElementById('photo'),button=document.getElementById('scan');
const result=document.getElementById('result'),status=document.getElementById('status');
const canvas=document.getElementById('photo-preview'),ctx=canvas.getContext('2d',{willReadFrequently:true});
let busy=false,pending=false,lastPending=false;
function updateButton(){button.disabled=busy||pending;photo.disabled=busy||pending;}
async function readStatus(){
  if(busy)return;
  try{
    const response=await fetch('/api/status',{cache:'no-store'});
    if(!response.ok)return;
    const data=await response.json();pending=data.pending;
    if(pending){
      result.textContent=`${data.fruit} · ${data.kcal} kcal`;
      status.textContent=`Standardportion: ${data.grams} g. Afventer dit valg på ESP32-skærmen. Intet er tilføjet endnu.`;
    }else if(lastPending){
      result.textContent='Valg behandlet på ESP32';
      status.textContent=`Registreret i dag: ${data.today} kcal. Du kan scanne igen.`;
    }
    lastPending=pending;
    button.disabled=busy||!data.can_scan;photo.disabled=busy||pending;
  }catch(error){/* Uploadfejl vises nedenfor; et forbindelsesudfald foretager ingen registrering. */}
}
button.addEventListener('click',async()=>{
  if(busy||pending)return;
  if(!photo.files.length){status.textContent='Tag eller vælg et billede først.';return;}
  busy=true;updateButton();result.textContent='Scanner…';status.textContent='Sender billedet og genkender frugten. Vent på resultatet.';
  const url=URL.createObjectURL(photo.files[0]);
  try{
    const img=new Image();img.src=url;await img.decode();
    const side=Math.min(img.width,img.height);
    ctx.drawImage(img,(img.width-side)/2,(img.height-side)/2,side,side,0,0,96,96);
    const rgba=ctx.getImageData(0,0,96,96).data,rgb=new Uint8Array(96*96*3);
    for(let i=0;i<96*96;i++){rgb[i*3]=rgba[i*4];rgb[i*3+1]=rgba[i*4+1];rgb[i*3+2]=rgba[i*4+2];}
    const form=new FormData();form.append('image',new Blob([rgb],{type:'application/octet-stream'}),'image.rgb');
    const response=await fetch('/upload',{method:'POST',body:form});
    const data=await response.json();if(!response.ok)throw new Error(data.error||'Billedet kunne ikke behandles.');
    if(!data.pending){result.textContent='Prøv igen';status.textContent=data.message+' Intet er registreret.';}
    else{pending=true;lastPending=true;result.textContent=data.fruit+' · '+data.kcal+' kcal';status.textContent=`Standardportion: ${data.grams} g. Vælg “Accepter” eller “Afvis” på ESP32-skærmen.`;}
  }catch(error){result.textContent='Scanning afbrudt';status.textContent=error.message+' Kontrollér ESP32-skærmen før et nyt forsøg.';}
  finally{URL.revokeObjectURL(url);busy=false;updateButton();await readStatus();}
});
async function poll(){await readStatus();setTimeout(poll,1000);}poll();
</script>
</html>
)html";
