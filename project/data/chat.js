state = -1;

document.addEventListener("DOMContentLoaded", function(e) {
  // Name can't be blank
  let user = ""
  while (user == "") {
    user = prompt("Please enter your name")
  }
  document.querySelector("#sender").value = user
  get_messages()
  const refreshInterval = setInterval(function() {
    get_messages()
  }, 1000)
  const form = document.querySelector("#chatForm")
  form.addEventListener("submit", function(e) {
    e.preventDefault()
    sendChat(e.target)
  })
})


function get_messages(){

	$.ajax({
		type: "GET",
		url: "http://127.0.0.1:8888/messages",
		success: function(data){
			lines = data.split("\n");
			console.log(lines);
			console.log(state);
			for (var i = state+1; i < (lines.length-1); i++) 
			{
                $('#chat-area').append($("<p>"+ lines[i] +"</p>"));
                state = i;
			}
			document.getElementById('chat-area').scrollTop = document.getElementById('chat-area').scrollHeight;
		},
	});

}

//send the message
function sendChat(form)
{       
    $.ajax(
    {
		type: "POST",
		url: "http://127.0.0.1:8888/messages",
		data: {
		   	sender: form.sender.value,
      		message: form.message.value
		},
		dataType: "json",
		success: function(data){
		},
	});

}