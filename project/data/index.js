state = 0;

document.addEventListener("DOMContentLoaded", function(e) {
  const attemptform = document.querySelector("#inputForm")
  attemptform.addEventListener("submit", function(e) {
    e.preventDefault()
    send_bid(e.target)
  })
  const winnerform = document.querySelector("#winnerForm")
  winnerform.addEventListener("submit", function(e) {
    e.preventDefault()
    get_winning_bid(e.target)
  })
})


function get_winning_bid(){

  $.ajax({
    type: "GET",
    url: "http://127.0.0.1:8888/game",
    success: function(data){
        console.log(data);
        $("#winner").val(data);
      }
    });
}

//send the message
function send_bid(form)
{       
    $.ajax({
      type: "POST",
      url: "http://127.0.0.1:8888/game",
      data: {
        value: form.message.value,
      },
      dataType: "json",
      success: function(data){
      },
    });

}