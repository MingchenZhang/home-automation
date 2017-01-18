function attach_common_button_handler(button, button_command, uuid_str, success_callback, success_callback_param){
    button.button_timeout = null;
    button.button_request = null;
    button.click(function(){
        if(button.button_request != null) button.button_request.abort();
        if(typeof button_command === 'function') button_command = button_command(button)
        button.button_request = $.ajax({
            type: "POST",
            url: "/module/" + uuid_str,
            data: button_command,
            dataType: "json",
            timeout: 10000,
            success: function(data) {
                if(data.result==='success'){
                    if(button.button_timeout != null){clearTimeout(button.button_timeout);button.button_timeout=null;}
                    button.attr('class','btn btn-success');
                    button.button_timeout = setTimeout(function(){button.attr('class','btn btn-default');}, 2000);
                    if (typeof success_callback_param !== 'undefined') { success_callback(success_callback_param); }
                    else{ success_callback(); }
                }else if(data.result==='no change'){
                    if(button.button_timeout != null){clearTimeout(button.button_timeout);button.button_timeout=null;}
                    button.attr('class','btn btn-primary');
                    button.button_timeout = setTimeout(function(){button.attr('class','btn btn-default');}, 2000);
                }else{
                    if(button.button_timeout != null){clearTimeout(button.button_timeout);button.button_timeout=null;}
                    button.attr('class','btn btn-danger');
                    button.button_timeout = setTimeout(function(){button.attr('class','btn btn-default');}, 2000);
                    $.bootstrapGrowl(data.result, { type: 'danger' });
                }
                button.button_request = null;
            },
            error: function(request, status, err) {
                if(status == "timeout") {
                    if(button.button_timeout != null){clearTimeout(button.button_timeout);button.button_timeout=null;}
                    button.attr('class','btn btn-warning');
                    button.button_timeout = setTimeout(function(){button.attr('class','btn btn-default');}, 2000);
                    $.bootstrapGrowl("server response timeout", { type: 'warning' });
                }else{
                    if(button.button_timeout != null){clearTimeout(button.button_timeout);button.button_timeout=null;}
                    button.attr('class','btn btn-danger');
                    button.button_timeout = setTimeout(function(){button.attr('class','btn btn-default');}, 2000);
                    $.bootstrapGrowl("unknown server connection error", { type: 'danger' });
                }
                button.button_request = null;
            }
        });
    });
};