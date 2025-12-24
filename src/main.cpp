#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

/* Register for Logs Begin */
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);
/* Register for Logs End */

/* Local defines Begin */
static constexpr uint8_t DebounceDelay = 20;
/* Local defines End */

/* Aliases definitions Begin */
#define SW0_NODE  DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define LED0_NODE DT_ALIAS(led0)
#if !DT_NODE_HAS_STATUS_OKAY(LED0_NODE)
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

#define LED1_NODE DT_ALIAS(led1)
#if !DT_NODE_HAS_STATUS_OKAY(LED1_NODE)
#error "Unsupported board: led1 devicetree alias is not defined"
#endif
/* Aliases definitions End */

/* GPIO definitions Begin */
static const struct device *const button_dev = DEVICE_DT_GET(DT_GPIO_CTLR(SW0_NODE, gpios));
static constexpr gpio_pin_t button_pin = DT_GPIO_PIN(SW0_NODE, gpios);
static constexpr gpio_flags_t button_flags = DT_GPIO_FLAGS(SW0_NODE, gpios);

static const struct device *const led0_dev = DEVICE_DT_GET(DT_GPIO_CTLR(LED0_NODE, gpios));
static constexpr gpio_pin_t led0_pin = DT_GPIO_PIN(LED0_NODE, gpios);
static constexpr gpio_flags_t led0_flags = DT_GPIO_FLAGS(LED0_NODE, gpios);

static const struct device *const led1_dev = DEVICE_DT_GET(DT_GPIO_CTLR(LED1_NODE, gpios));
static constexpr gpio_pin_t led1_pin = DT_GPIO_PIN(LED1_NODE, gpios);
static constexpr gpio_flags_t led1_flags = DT_GPIO_FLAGS(LED1_NODE, gpios);
/* GPIO definitions End */

/* ISR Definitions Begin */
static struct gpio_callback button_cb;
/* ISR Definitions End */

/* Work Queues Definitions Begin */
static struct k_work_delayable button_work;
/* Work Queues Definitions End */

/* Used Variables Begin */
static bool led_on = false;
/* Used Variables End */

/* Work Queues Functions Begin */
static void button_work_handler(struct k_work *work);
/* Work Queues Functions Definitions End */

/* ISR Functions Begin */
static void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins);
/* ISR Functions End */

int main(void)
{
    /* Check the device Begin */
    if (!device_is_ready(button_dev)) 
    {
        LOG_ERR("Button device not ready");
        return 0;
    }

    if (!device_is_ready(led0_dev)) 
    {
        LOG_ERR("LED device not ready");
        return 0;
    }

    if (!device_is_ready(led1_dev)) 
    {
        LOG_ERR("LED1 device not ready");
        return 0;
    }
    /* Check the device End */

    /* GPIO configurations Begin */
    int r = gpio_pin_configure(button_dev, button_pin, GPIO_INPUT | button_flags);
    if (r) 
    {
        LOG_ERR("Button configure failed: %d", r);
        return 0;
    }

    r = gpio_pin_configure(led0_dev, led0_pin, GPIO_OUTPUT_INACTIVE | led0_flags);
    if (r) 
    {
        LOG_ERR("LED configure failed: %d", r);
        return 0;
    }

    r = gpio_pin_configure(led1_dev, led1_pin, GPIO_OUTPUT_INACTIVE | led1_flags);
    if (r)
    {
        LOG_ERR("LED1 configure failed: %d", r);
        return 0;
    }
    /* GPIO configurations End */

    /* Initialize Work Queue Begin */
    k_work_init_delayable(&button_work, button_work_handler);
    /* Initialize Work Queue End */

    /* GPIO ISR Configurations Begin */
    gpio_init_callback(&button_cb, button_isr, BIT(button_pin));
    gpio_add_callback(button_dev, &button_cb);

    r = gpio_pin_interrupt_configure(button_dev, button_pin, GPIO_INT_EDGE_TO_ACTIVE);
    if (r) 
    {
        LOG_ERR("Interrupt configure failed: %d", r);
        return 0;
    }
    /* GPIO ISR Configurations End */

    /* Sleep the main thread to not overload the CPU */
    k_sleep(K_FOREVER);

    return 0;
}

static void button_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);

    int val = gpio_pin_get(button_dev, button_pin);
    if (val < 0) 
    {
        LOG_ERR("gpio_pin_get failed: %d", val);
        return;
    }

    if (val) 
    {
        led_on = !led_on;
        int r = gpio_pin_set(led0_dev, led0_pin, led_on ? 1 : 0);
        if (r < 0) 
        {
            LOG_ERR("gpio_pin_set failed: %d", r);
        } 
        else 
        {
            LOG_INF("LED0 toggled: %d", (int)led_on);
        }
    }
}

static void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    (void)k_work_reschedule(&button_work, K_MSEC(DebounceDelay));
}