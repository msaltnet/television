 /*
 * Copyright (c) 2018 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

window.onload = function(){
    var container = document.querySelector("#camera-view");
    var canvas = new Canvas("camera-view-canvas", container.offsetWidth, container.offsetHeight);
    runWebSocket();

    function runWebSocket() {
        var wsUri = "ws://" + window.location.hostname + ":8888/";

        websocket = new WebSocket(wsUri);
        websocket.onopen = function(evt) { onOpen(evt) };
        websocket.onclose = function(evt) { onClose(evt) };
        websocket.onmessage = function(evt) { onMessage(evt) };
        websocket.onerror = function(evt) { onError(evt) };
    }

    function onOpen(evt)
    {
        console.log("CONNECTED");
        doSend("HELLO FROM BROWSER via WebSocket!!!!!!!!!!!!");
    }

    function onClose(evt)
    {
        console.log("DISCONNECTED");
    }

    function onMessage(evt)
    {
        var urlCreator = window.URL || window.webkitURL;
        var imageUrl = urlCreator.createObjectURL(evt.data);
        document.querySelector("#camera-view").src = imageUrl;

        var arrayBuffer;
        var fileReader = new FileReader();
        fileReader.onload = function(event) {
            arrayBuffer = event.target.result;
            var exif = EXIF.readFromBinaryFile(arrayBuffer);
            var exifInfoString = asciiToStr(exif.UserComment, 8);
            var type = 'blur';
            if (getResultType(exifInfoString) != 0) {
                type = 'active';
            }

            var pointArray = getPointArrayFromString(exifInfoString.slice(4));

            canvas.clearPoints();
            canvas.drawPoints(pointArray, type);
        };
        fileReader.readAsArrayBuffer(evt.data);

        doSend("ack", true);
    }

    function onError(evt)
    {
        console.log(evt.data);
    }

    function doSend(message, dont_print_log)
    {
        websocket.send(message);
        if (!dont_print_log)
            console.log("SENT: " + message);
    }


    function asciiToStr(asciiArr, start) {
        var string = "";
        var i = start;

        for (; i < asciiArr.length; i++) {
            string += String.fromCharCode(asciiArr[i]);
        }

        return string;
    }

    function getResultType(str) {
        return Number(str.slice(0, 2));
    }

    function getPointArrayFromString(pointStr) {
        var strLen = pointStr.length;
        var i = 0;
        var pointArray = [];

        while (i < strLen) {
            pointArray.push(pointStr.slice(i, i + 8));
            i += 8;
        }

        return pointArray;
    }
};

function Canvas(canvasId, width, height) {
    this.width = width;
    this.height = height;
    this.viewCanvas = document.getElementById(canvasId);
    this.viewContext = this.viewCanvas.getContext("2d");
    this.viewContext.canvas.width = this.width;
    this.viewContext.canvas.height = this.height;
}

Canvas.prototype.clearCanvas = function() {
    this.viewContext.clearRect(0,0, this.width, this.height);
}

Canvas.prototype.clearPoints = function () {
    this.clearCanvas();
}

Canvas.prototype.drawRect = function(x, y, width, height, color) {
    // console.log(`x: ${x}, y: ${y}, w: ${width}, h: ${height}, c: ${color}`);
    this.viewContext.beginPath();
    this.viewContext.moveTo(x, y);
    this.viewContext.lineTo(x, y + height);
    this.viewContext.lineTo(x + width, y + height);
    this.viewContext.lineTo(x + width, y);
    this.viewContext.closePath();
    this.viewContext.lineWidth = 3;
    this.viewContext.strokeStyle = color;
    this.viewContext.stroke();
}

Canvas.prototype.drawPoints = function (pointArray, type) {
    var i = 0;
    var x, y, w, h;
    var color;

    // 실습 - IoTjs 사각형 색상 바꿔보기
    if (type == 'blur') {
        color = "rgba(115,232,57,0.8)";
    } else {
        color = "rgba(255,0,0, 0.8)";
    }

    for (i = 0; i < pointArray.length; i++) {
        x = this.width / 99 * parseInt(pointArray[i].slice(0,2));
        y = this.height / 99 * parseInt(pointArray[i].slice(2,4));
        w = this.width / 99 * parseInt(pointArray[i].slice(4,6));
        h = this.height / 99 * parseInt(pointArray[i].slice(6,8));

        this.drawRect(x, y, w, h, color);
    }
}
