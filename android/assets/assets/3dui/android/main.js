
var timePassed = 0;

room.update = function(dt) {

  timePassed += 1*(dt/10);

  room.objects["foam"].scale.x += (Math.sin(timePassed*0.01)*0.001);
  room.objects["foam"].scale.z += (Math.sin(timePassed*0.01)*0.001);

  if (player.xbox.connected && room.objects["controls"].image_id != "controlsGamepad") {
      room.objects["controls"].image_id = "controlsGamepad";
  }
  else if ((player.hmd_type == "daydream" || player.hmd_type == "cardboard") && room.objects["controls"].image_id != "controlsDaydream"){
    room.objects["controls"].image_id = "controlsDaydream";
  }
  else if (player.hmd_type == "gear" && room.objects["controls"].image_id != "controlsGear"){
    room.objects["controls"].image_id = "controlsGear";
  }
  else if (player.hmd_type == "go" && room.objects["controls"].image_id != "controlsGo"){
    room.objects["controls"].image_id = "controlsGo";
  }
  else if (player.hmd_type == "2d" && room.objects["controls"].image_id != "controls2d"){
	room.objects["controls"].image_id = "controls2d";
  }
}
