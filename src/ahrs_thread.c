#include <ch.h>
#include "main.h"
#include "MadgwickAHRS.h"
#include "ahrs_thread.h"
#include "imu_thread.h"

static struct {
    parameter_t beta;
    parameter_namespace_t ns;
} ahrs_params;

static void ahrs_thd(void *p)
{
    (void) p;
    chRegSetThreadName("AHRS");

    parameter_namespace_declare(&ahrs_params.ns, &parameter_root, "ahrs");
    parameter_scalar_declare_with_default(&ahrs_params.beta, &ahrs_params.ns, "beta", 0.1);

    messagebus_topic_t *imu_topic;

    imu_topic = messagebus_find_topic_blocking(&bus, "/imu");

    /* Create the attitude topic. */
    messagebus_topic_t attitude_topic;
    MUTEX_DECL(attitude_topic_lock);
    CONDVAR_DECL(attitude_topic_condvar);
    attitude_msg_t attitude_topic_content;

    messagebus_topic_init(&attitude_topic,
                          &attitude_topic_lock, &attitude_topic_condvar,
                          &attitude_topic_content, sizeof(attitude_topic_content));
    messagebus_advertise_topic(&bus, &attitude_topic, "/attitude");

    while (1) {
        if (parameter_changed(&ahrs_params.beta)) {
            madgwick_filter_set_gain(parameter_scalar_get(&ahrs_params.beta));
        }

        imu_msg_t imu;
        messagebus_topic_wait(imu_topic, &imu, sizeof(imu));

        magdwick_filter_update(imu.gyro.x, imu.gyro.y, imu.gyro.z,
                               imu.acc.x, imu.acc.y, imu.acc.z,
                               imu.mag.x, imu.mag.y, imu.mag.z);

        attitude_msg_t msg;

        madgwick_filter_get_quaternion(&msg.q[0]);

        messagebus_topic_publish(&attitude_topic, &msg, sizeof(msg));
    }
}

void ahrs_start(void)
{
    static THD_WORKING_AREA(ahrs_thd_wa, 2048);
    chThdCreateStatic(ahrs_thd_wa, sizeof(ahrs_thd_wa),
                      NORMALPRIO, ahrs_thd, NULL);
}