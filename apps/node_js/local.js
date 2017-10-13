//#0      ACT_SWITCH      LED(orange)
//#1      SENSE_BTN       Button(SW0)
//#2      ACT_ANY PWM(PA16)
//#3      ACT_ANY PWM(PA18)
//#4      SENSE_ANALOG    ADC0_1(PA06)
//#5      SENSE_ANALOG    ADC0_2(PA07)

saul.get_by_name = function (name) {
    var res;
    switch (name) {
        case "led":
            return saul._find_name("LED(orange)");
        case "button":
            res = saul._find_name("Button(SW0)");
            break;
        case "buzzer":
            res = saul._find_name("PWM(PA18)");
            break;
        case "brightness":
            res = saul._find_name("ADC0_1(PA06)");
            break;
        case "sound":
            res = saul._find_name("ADC0_2(PA07)");
            break;
    }
    saul.set_methods(res);
    return res;
}

button = saul.get_by_name("button");

coap = {
    request : function(url, method, data) { print("fake COAP REQUEST SENT"); },
    register_handler : function (url, method, callback) { button.on_threshold(0, callback, saul.op.EQ); }
}
