var canvasw = 1080
var canvash = 648
var factor = 1.0;
var winlist = []
var lastmaxx;
var lastminx;
var lastmaxy;
var lastminy;
function draw(canvas, winlist)
{
    var ww = canvas.width;
    var wh = canvas.height;
    let maxx = Number.MIN_VALUE;
    let minx = Number.MAX_VALUE;
    let maxy = Number.MIN_VALUE;
    let miny = Number.MAX_VALUE;
    for (const win of winlist) {
        console.log(`x ${win["x"]}, y ${win["y"]}, w ${win["w"]}, w ${win["h"]}, `)
        if(maxx < win["x"] + win["w"]){
            maxx = win["x"] + win["w"]
        }
        if(minx > win["x"]){
            minx = win["x"]
        }
        if(maxy < win["y"] + win["h"]){
            maxy = win["y"] + win["h"]
        }
        if(miny > win["y"]){
            miny = win["y"]
        }
    }
    lastmaxx = maxx;
    lastminx = minx;
    lastmaxy = maxy;
    lastminy = miny;
    console.log(`maxx ${maxx}, minx ${minx}, maxy ${maxy}, miny ${miny}, `)
    factor = Math.min(ww/(maxx-minx),wh/( maxy-miny))
    console.log(`factor ${factor}`)
    if (canvas.getContext) {
        var ctx = canvas.getContext("2d");
        
        ctx.clearRect(0, 0, ww, wh);
        var img = new Image();
        img.src="data:image/svg+xml;base64,PD94bWwgdmVyc2lvbj0iMS4wIiA/PjxzdmcgY2xhc3M9ImZlYXRoZXIgZmVhdGhlci1jaHJvbWUiIGZpbGw9Im5vbmUiIGhlaWdodD0iMjQiIHN0cm9rZT0iY3VycmVudENvbG9yIiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiIHN0cm9rZS13aWR0aD0iMiIgdmlld0JveD0iMCAwIDI0IDI0IiB3aWR0aD0iMjQiIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+PGNpcmNsZSBjeD0iMTIiIGN5PSIxMiIgcj0iMTAiLz48Y2lyY2xlIGN4PSIxMiIgY3k9IjEyIiByPSI0Ii8+PGxpbmUgeDE9IjIxLjE3IiB4Mj0iMTIiIHkxPSI4IiB5Mj0iOCIvPjxsaW5lIHgxPSIzLjk1IiB4Mj0iOC41NCIgeTE9IjYuMDYiIHkyPSIxNCIvPjxsaW5lIHgxPSIxMC44OCIgeDI9IjE1LjQ2IiB5MT0iMjEuOTQiIHkyPSIxNCIvPjwvc3ZnPg=="
        images = {}
        images["google-chrome.Google-chrome"]=img
        for (const win of winlist) {
            let x  = (win["x"] - minx) * factor
            let y  = (win["y"] - miny) * factor
            let w  = win["w"] * factor
            let h  = win["h"] * factor
            ctx.fillStyle = "rgb(200,0,0,0.3)";
            if(win["focused"]>0){
                ctx.fillRect(x, y, w, h);
            }
            ctx.strokeRect(x, y, w, h);
            
            ctx.fillStyle = "rgba(0, 0, 200)";
            ctx.font = "20px serif";
            let name = win["name"]
            var mtext = ctx.measureText(name);
            while(mtext.width > w) 
            {
                name = name.substring(0, name.length -1)
                mtext = ctx.measureText(name);
            }
            
            ctx.fillText(name, x+w/2-mtext.width/2, y+h/2+30);
            
            if(images[win["class"]]){
                ctx.drawImage(images[win["class"]], x+w/2, y+h/2-30);
            }
            
        }
    }
}

function getwin(x, y, winlist){
    for (const win of winlist) {
        if(x < win["x"]+win["w"] && x > win["x"] && y < win["y"]+win["h"] && y > win["y"] )
            return win
    }
    return null
}

function init() {
    let canvas = document.getElementById("windowswitcher");
    fetch('/list', {
        method: 'POST',
        body: JSON.stringify({}),
        headers: {
        'Content-Type': 'application/json'
        }
    })
    .then(res => res.text())
    .then(text=>JSON.parse(text))
    .then(data =>{
        winlist= data["content"]
        lastwinlist = winlist
        draw(canvas, data["content"])
    })
    .catch(err => console.error(err))

    canvas.addEventListener('click', (e) => {
        let x =  e.clientX
        let y =  e.clientY
        let win = getwin(x/factor+lastminx, y/factor+lastminy,winlist)
        console.log('canvas click '+win);
        if(win){
            for (const w of lastwinlist) {
                if(w["focused"] == 1){
                    w["focused"] = 0
                }
            }
            for (const w of lastwinlist) {
                if(w["wid"] == win["wid"]){
                    w["focused"] = 1
                    break
                }
            }
            draw(canvas, lastwinlist)
            fetch('/focus', {
                method: 'POST',
                body: JSON.stringify({"wid":win["wid"]}),
                headers: {
                'Content-Type': 'application/json'
                }
            })
            .then(res => res.text())
            .then(text=>JSON.parse(text))
            .then(data =>{
                winlist= data["content"]
                draw(canvas, data["content"])
            })
            .catch(err => console.error(err))
        }
        console.log('canvas click'+x+","+y);
    });

    setInterval(() => {
        fetch('/list', {
            method: 'POST',
            body: JSON.stringify({}),
            headers: {
            'Content-Type': 'application/json'
            }
        })
        .then(res => res.text())
        .then(text=>JSON.parse(text))
        .then(data =>{
            winlist= data["content"]
            lastwinlist = winlist
            draw(canvas, data["content"])
        })
        .catch(err => console.error(err))
    }, 1000*1);
}

function hotkey(key){
    fetch('/hotkey', {
        method: 'POST',
        body: JSON.stringify({"hotkey":key}),
        headers: {
        'Content-Type': 'application/json'
        }
    })
    .then(res => res.text())
    .then(text=>JSON.parse(text))
    .then(data =>{ })
}


function command(cmd){
    fetch('/command', {
        method: 'POST',
        body: JSON.stringify({"command":cmd}),
        headers: {
        'Content-Type': 'application/json'
        }
    })
    .then(res => res.text())
    .then(text=>JSON.parse(text))
    .then(data =>{ })
}

function fullscreen() {
    hotkey("alt+shift+m")
}

function enter() {
    hotkey("0xff0d")
}

function left() {
    hotkey("0xff51")
}

function right() {
    hotkey("0xff53")
}

function up() {
    hotkey("0xff52")
}
function down() {
    hotkey("0xff54")
}
function F1() {
    hotkey("0xffbe")
}
