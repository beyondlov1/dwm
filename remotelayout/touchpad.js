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


// 获取触摸板元素
const touchpad = document.getElementById('touchpad');

// 记录触摸起始位置
let startX = 0;
let startY = 0;

// 处理触摸开始事件
touchpad.addEventListener('touchstart', function(event) {
  // 记录触摸起始位置
  startX = event.touches[0].clientX;
  startY = event.touches[0].clientY;
});

// 处理触摸移动事件
touchpad.addEventListener('touchmove', function(event) {
  // 计算触摸移动距离
  const deltaX = event.touches[0].clientX - startX;
  const deltaY = event.touches[0].clientY - startY;

  // 根据移动距离执行相应操作
  if (Math.abs(deltaX) > Math.abs(deltaY)) {
    // 水平滑动
    if (deltaX > 0) {
      // 向右滑动
      // console.log('向右滑动');
    } else {
      // 向左滑动
      // console.log('向左滑动');
    }
  } else {
    // 垂直滑动
    if (deltaY > 20) {
      // 向下滑动
      // console.log('向下滑动');
      hotkey("0xff52")
      startY = startY + deltaY;
    } else if (deltaY < -20){
      // 向上滑动
      // console.log('向上滑动');
      hotkey("0xff54")
      startY = startY + deltaY;
    }
  }

  // 阻止默认滚动行为
  event.preventDefault();
});

// 处理触摸结束事件
touchpad.addEventListener('touchend', function(event) {
  // 处理点击事件
  if (event.changedTouches.length === 1) {
    console.log('点击');
  }
});



// // 处理鼠标按下事件
// touchpad.addEventListener('mousedown', (event) => {
//     console.log(`Mouse Down: X: ${event.clientX}, Y: ${event.clientY}`);
// });
//
// // 处理鼠标移动事件
// touchpad.addEventListener('mousemove', (event) => {
//     console.log(`Mouse Move: X: ${event.clientX}, Y: ${event.clientY}`);
// });
//
// // 处理鼠标释放事件
// touchpad.addEventListener('mouseup', (event) => {
//     console.log('Mouse Up');
// });
