#include <gtk/gtk.h>

struct ss_config {
	double seg_wt, border_wt;
	double on_r, on_g, on_b;
	double off_r, off_g, off_b;
	double label_r, label_g, label_b;
	int __height, __width, __seg_len;
	int text_pt;
};

static struct ss_config utc_clock_config = { .seg_wt = 25,
					     .border_wt = 5,
					     .on_r = 1,
					     .on_g = 0,
					     .on_b = 0,
					     .off_r = 0.2,
					     .off_g = 0,
					     .off_b = 0,
					     .text_pt = 52,
					     .label_r = 1,
					     .label_g = 1,
					     .label_b = 1 };

static struct ss_config local_clock_config = { .seg_wt = 25,
					       .border_wt = 5,
					       .on_r = 0,
					       .on_g = 0,
					       .on_b = 1,
					       .text_pt = 52,
					       .off_r = 0,
					       .off_g = 0,
					       .off_b = 0.1,
					       .label_r = 1,
					       .label_g = 1,
					       .label_b = 1 };

static int sevenseg_vals[10] = { 0b1110111, 0b0010010, 0b1011101, 0b1011011,
				 0b0111010, 0b1101011, 0b1101111, 0b1010010,
				 0b1111111, 0b1111011 };

static void draw_h_seg(cairo_t *cr, double x0, double y0, int on,
		       const struct ss_config *conf)
{
	cairo_move_to(cr, x0, y0);
	cairo_rel_line_to(cr, conf->seg_wt, -1 * conf->seg_wt);
	cairo_rel_line_to(cr, conf->__seg_len, 0);
	cairo_rel_line_to(cr, conf->seg_wt, conf->seg_wt);
	cairo_rel_line_to(cr, -1 * conf->seg_wt, conf->seg_wt);
	cairo_rel_line_to(cr, -1 * conf->__seg_len, 0);
	cairo_close_path(cr);

	cairo_set_line_width(cr, conf->border_wt);
	if (on)
		cairo_set_source_rgb(cr, conf->on_r, conf->on_g, conf->on_b);
	else
		cairo_set_source_rgb(cr, conf->off_r, conf->off_g, conf->off_b);
	cairo_fill_preserve(cr);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_stroke(cr);
}

static void draw_v_seg(cairo_t *cr, double x0, double y0, int on,
		       const struct ss_config *conf)
{
	cairo_move_to(cr, x0, y0);
	cairo_rel_line_to(cr, conf->seg_wt, conf->seg_wt);
	cairo_rel_line_to(cr, 0, conf->__seg_len);
	cairo_rel_line_to(cr, -1 * conf->seg_wt, conf->seg_wt);
	cairo_rel_line_to(cr, -1 * conf->seg_wt, -1 * conf->seg_wt);
	cairo_rel_line_to(cr, 0, -1 * conf->__seg_len);
	cairo_close_path(cr);

	cairo_set_line_width(cr, conf->border_wt);
	if (on)
		cairo_set_source_rgb(cr, conf->on_r, conf->on_g, conf->on_b);
	else
		cairo_set_source_rgb(cr, conf->off_r, conf->off_g, conf->off_b);
	cairo_fill_preserve(cr);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_stroke(cr);
}

static void draw_full_seven_seg(cairo_t *cr, double x0, double y0, int value,
				const struct ss_config *conf)
{
	value %= 10;

	int mask = sevenseg_vals[value];

	draw_h_seg(cr, x0 + conf->seg_wt, y0 + conf->seg_wt, mask & 64, conf);
	draw_v_seg(cr, x0 + conf->seg_wt, y0 + conf->seg_wt, mask & 32, conf);
	draw_v_seg(cr, x0 + conf->__seg_len + 3 * conf->seg_wt,
		   y0 + conf->seg_wt, mask & 16, conf);
	draw_h_seg(cr, x0 + conf->seg_wt,
		   y0 + conf->__seg_len + conf->seg_wt * 3, mask & 8, conf);
	draw_v_seg(cr, x0 + conf->seg_wt,
		   y0 + conf->__seg_len + conf->seg_wt * 3, mask & 4, conf);
	draw_v_seg(cr, x0 + conf->__seg_len + 3 * conf->seg_wt,
		   y0 + conf->__seg_len + 3 * conf->seg_wt, mask & 2, conf);
	draw_h_seg(cr, x0 + conf->seg_wt,
		   y0 + conf->__seg_len * 2 + conf->seg_wt * 5, mask & 1, conf);
}

static void draw_colon_box(cairo_t *cr, double x0, double y0, int on,
			   const struct ss_config *conf)
{
	cairo_move_to(cr, x0, y0);
	cairo_rel_line_to(cr, conf->seg_wt, 0);
	cairo_rel_line_to(cr, 0, conf->seg_wt);
	cairo_rel_line_to(cr, -1 * conf->seg_wt, 0);
	cairo_close_path(cr);

	cairo_set_line_width(cr, conf->border_wt);
	if (1)
		cairo_set_source_rgb(cr, conf->on_r, conf->on_g, conf->on_b);
	else
		cairo_set_source_rgb(cr, conf->off_r, conf->off_g, conf->off_b);
	cairo_fill_preserve(cr);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_stroke(cr);
}

static void draw_colon(cairo_t *cr, double x0, double y0, int on,
		       const struct ss_config *conf)
{
	draw_colon_box(cr, x0, y0 + (0.8 * conf->__seg_len) - conf->seg_wt, on,
		       conf);
	draw_colon_box(cr, x0,
		       y0 + conf->__height - (0.8 * conf->__seg_len) -
			       conf->seg_wt,
		       on, conf);
}

static void draw_centered_text(cairo_t *cr, const char *text, double cx,
			       double ty, const struct ss_config *conf)
{
	cairo_text_extents_t extents;

	cairo_text_extents(cr, text, &extents);

	double x = cx - (extents.width / 2 + extents.x_bearing);
	double y = ty - extents.y_bearing;

	cairo_move_to(cr, x, y);
	cairo_set_source_rgb(cr, conf->label_r, conf->label_g, conf->label_b);
	cairo_show_text(cr, text);
}

static void draw_clock(cairo_t *cr, double y0, int h, int m, int s,
		       const struct ss_config *conf)
{
	double x =
		(conf->__width - 33 * conf->seg_wt - 6 * conf->__seg_len) / 2;
	double ss_width = conf->__seg_len + 5 * conf->seg_wt;

	draw_full_seven_seg(cr, x, y0, h / 10, conf);
	x += ss_width;
	draw_full_seven_seg(cr, x, y0, h % 10, conf);
	x += ss_width;

	draw_colon(cr, x, y0, 1, conf);
	x += 2 * conf->seg_wt;

	draw_full_seven_seg(cr, x, y0, m / 10, conf);
	x += ss_width;
	draw_full_seven_seg(cr, x, y0, m % 10, conf);
	x += ss_width;

	draw_colon(cr, x, y0, 1, conf);
	x += 2 * conf->seg_wt;

	draw_full_seven_seg(cr, x, y0, s / 10, conf);
	x += ss_width;
	draw_full_seven_seg(cr, x, y0, s % 10, conf);
	x += ss_width;
}

static void draw_hms_labels(cairo_t *cr, double y0,
			      const struct ss_config *conf)
{
	double center = conf->__width / 2;
	double offset = 12 * conf->seg_wt + 2 * conf->__seg_len;

	draw_centered_text(cr, "HOURS", center - offset, y0, conf);
	draw_centered_text(cr, "MINUTES", center, y0, conf);
	draw_centered_text(cr, "SECONDS", center + offset, y0, conf);
}

struct time_info {
    int utc_h, utc_m, utc_s;
    int local_h, local_m, local_s;
    int utc_offset;
};

static void get_time_info(struct time_info *info) {
    GDateTime *utc, *local;
    GTimeSpan utc_difference;

    utc = g_date_time_new_now_utc();
    info->utc_h = g_date_time_get_hour(utc);
    info->utc_m = g_date_time_get_minute(utc);
    info->utc_s = g_date_time_get_second(utc);

    local = g_date_time_new_now_local();
    info->local_h = g_date_time_get_hour(local);
    info->local_m = g_date_time_get_minute(local);
    info->local_s = g_date_time_get_second(local);

    utc_difference = g_date_time_difference(utc, local);
    info->utc_offset = g_date_time_get_utc_offset(local) / 3600000000LL;

    g_date_time_unref(utc);
    g_date_time_unref(local);
}

static void draw(GtkDrawingArea *area, cairo_t *cr, int width, int height,
		 gpointer data)
{
    cairo_text_extents_t title_extents, hms_extents, local_extents;
	double y_offset, x_offset;
	struct ss_config *conf = &utc_clock_config;
    GDateTime *utc = g_date_time_new_now_utc(), *local = g_date_time_new_now_local();
	conf->__width = width;
    GString *localtime_label = g_string_new("");

	cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
			       CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr, conf->text_pt);

    struct time_info tinfo;
    get_time_info(&tinfo);

    g_string_printf(localtime_label, "%s TIME (UTC %d)", tinfo.utc_offset == -6 ? "CENTRAL STANDARD" : tinfo.utc_offset == -5 ? "CENTRAL DAYLIGHT" : "LOCAL", tinfo.utc_offset);

    cairo_text_extents(cr, "COORDINATED UNIVERSAL TIME", &title_extents);
    cairo_text_extents(cr, "HOURS MINUTES SECONDS", &hms_extents);
    cairo_text_extents(cr, localtime_label->str, &local_extents);

    conf->__height = (height - title_extents.height - hms_extents.height - local_extents.height - 6 * conf->seg_wt) / 2;

    int v_len = (conf->__height / 2) - (conf->seg_wt * 3);
    int h_len = (conf->__width - 35 * conf->seg_wt) / 6;
	conf->__seg_len = h_len < v_len ? h_len : v_len;

	cairo_move_to(cr, 0, 0);
	cairo_rel_line_to(cr, width, 0);
	cairo_rel_line_to(cr, 0, height);
	cairo_rel_line_to(cr, -1 * width, 0);
	cairo_close_path(cr);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_fill(cr);

	x_offset = 0;
	y_offset = conf->seg_wt;

	draw_centered_text(cr, "COORDINATED UNIVERSAL TIME", width / 2,
			   y_offset, conf);
	y_offset += title_extents.height + conf->seg_wt;

	draw_clock(cr, y_offset, g_date_time_get_hour(utc), g_date_time_get_minute(utc), g_date_time_get_second(utc), conf);
	y_offset += conf->__height + conf->seg_wt;

    draw_hms_labels(cr, y_offset, conf);
	y_offset += hms_extents.height;
	y_offset += conf->seg_wt;

	local_clock_config.__height = conf->__height;
	local_clock_config.__width = conf->__width;
	local_clock_config.__seg_len = conf->__seg_len;

	draw_clock(cr, y_offset, g_date_time_get_hour(local), g_date_time_get_minute(local), g_date_time_get_second(local), &local_clock_config);
    y_offset += conf->__height + conf->seg_wt;

    draw_centered_text(cr, localtime_label->str, width / 2, y_offset, conf);

    g_string_free(localtime_label, true);
}

static gboolean update_clock(gpointer user_data)
{
    GtkWidget *utclock = user_data;

    gtk_widget_queue_draw(utclock);

    GDateTime *now = g_date_time_new_now_utc();
    int millis = g_date_time_get_microsecond(now) / 1000;
    int to_go = 1000 - millis + 1;
    // 1ms buffer makes sure we are squarely in the next second next time we update
    // nobody will notice

    g_timeout_add(to_go, update_clock, utclock);

    return FALSE;
}

static void on_activate(GtkApplication *app, gpointer user_data)
{
	GtkWidget *window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "utc-clock");
	//gtk_window_set_decorated(GTK_WINDOW(window), false);
	//gtk_window_maximize(GTK_WINDOW(window));
    gtk_window_fullscreen(GTK_WINDOW(window));

	GtkWidget *utclock = gtk_drawing_area_new();
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(utclock), draw, NULL,
				       NULL);
	gtk_window_set_child(GTK_WINDOW(window), GTK_WIDGET(utclock));

	gtk_window_present(GTK_WINDOW(window));

    g_timeout_add(500, update_clock, utclock);
}

int main(int argc, char **argv, char **envp)
{
	GtkApplication *app;
	int status;

	app = gtk_application_new("com.micromashor.utc-clock",
				  G_APPLICATION_FLAGS_NONE);

	g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}