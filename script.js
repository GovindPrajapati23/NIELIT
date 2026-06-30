setInterval(drawClock,2000);

function drawClock(){

let now=new Date();

let hour=now.getHours();
let minute=now.getMinutes();

let options={
year:'numeric',
month:'long',
day:'numeric'
};

document.getElementById("date").innerHTML=
now.toLocaleDateString("en-US",options);

let hourAngle=(360*(hour/12))+((360/12)*(minute/60));

let minAngle=360*(minute/60);

document.getElementById("hour").style.transform=
"rotate("+hourAngle+"deg)";

document.getElementById("min").style.transform=
"rotate("+minAngle+"deg)";

fetch("/readADC")
.then(res=>res.json())
.then(data=>{

document.getElementById("temperature").innerHTML=Math.round(data.Temperature)+"°C";

document.getElementById("temp").innerHTML=Math.round(data.Temperature)+"°C";

document.getElementById("humidity").innerHTML=Math.round(data.Humidity)+"%";

document.getElementById("pressure").innerHTML=Math.round(data.Pressuremb)+" mb";

if (data.Rain == 100) {
    document.getElementById("rain").innerHTML = "RAIN";
} else {
    document.getElementById("rain").innerHTML = "NO RAIN";
}

document.getElementById("mq135").innerHTML=data.MQ135;

});

}