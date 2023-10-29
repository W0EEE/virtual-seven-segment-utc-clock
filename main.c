#include <gtk/gtk.h>

#define DISPLAY_WIDTH  1920
#define DISPLAY_HEIGHT 1080

struct rgb {
	double r, g, b;
};

struct ss_colors {
	struct rgb on, off;
};

struct ss_geometry {
	double seg_wt, border_wt, seg_len;
};

struct ss_colon {
	GtkWidget *drawing_area;
	const struct ss_geometry *geometry;
	const struct rgb *color;
};

struct ss_digit {
	GtkWidget *drawing_area;
	const struct ss_geometry *geometry;
	const struct ss_colors *colors;
	int value;
};

struct text_config {
	int text_pt;
};

static struct config {
	struct text_config labels;
	struct ss_colors c_utc, c_local;
	double seg_wt, border_wt;
} app_config = { .labels = { .text_pt = 1024 * 48 },
		 .c_utc = { { 1, 0, 0 }, { 0.1, 0, 0 } },
		 .c_local = { { 0, 0, 1 }, { 0, 0, 0.1 } },
		 .seg_wt = 25,
		 .border_wt = 5 };

struct ss_clock {
	struct ss_digit *digits[6];
	struct ss_colon *colons[2];
};

struct time_info {
	int utc_h, utc_m, utc_s;
	int local_h, local_m, local_s;
	int utc_offset;
};

struct clockapp {
	GtkWidget *grid;
	GtkWidget *l_utc, *l_local, *l_hours, *l_minutes, *l_seconds;
	struct ss_clock utc, local;
	struct ss_geometry geometry;
	int prev_utc_offset;
};

static int sevenseg_vals[10] = { 0b1110111, 0b0010010, 0b1011101, 0b1011011,
				 0b0111010, 0b1101011, 0b1101111, 0b1010010,
				 0b1111111, 0b1111011 };

static void draw_h_seg(cairo_t *cr, int on, const struct ss_digit *conf)
{
	cairo_rel_line_to(cr, conf->geometry->seg_wt,
			  -1 * conf->geometry->seg_wt);
	cairo_rel_line_to(cr, conf->geometry->seg_len, 0);
	cairo_rel_line_to(cr, conf->geometry->seg_wt, conf->geometry->seg_wt);
	cairo_rel_line_to(cr, -1 * conf->geometry->seg_wt,
			  conf->geometry->seg_wt);
	cairo_rel_line_to(cr, -1 * conf->geometry->seg_len, 0);
	cairo_close_path(cr);

	const struct rgb *fill = on ? &conf->colors->on : &conf->colors->off;

	cairo_set_line_width(cr, conf->geometry->border_wt);
	cairo_set_source_rgb(cr, fill->r, fill->g, fill->b);
	cairo_fill_preserve(cr);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_stroke(cr);
}

static void draw_v_seg(cairo_t *cr, int on, const struct ss_digit *conf)
{
	cairo_rel_line_to(cr, conf->geometry->seg_wt, conf->geometry->seg_wt);
	cairo_rel_line_to(cr, 0, conf->geometry->seg_len);
	cairo_rel_line_to(cr, -1 * conf->geometry->seg_wt,
			  conf->geometry->seg_wt);
	cairo_rel_line_to(cr, -1 * conf->geometry->seg_wt,
			  -1 * conf->geometry->seg_wt);
	cairo_rel_line_to(cr, 0, -1 * conf->geometry->seg_len);
	cairo_close_path(cr);

	const struct rgb *fill = on ? &conf->colors->on : &conf->colors->off;

	cairo_set_line_width(cr, conf->geometry->border_wt);
	cairo_set_source_rgb(cr, fill->r, fill->g, fill->b);
	cairo_fill_preserve(cr);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_stroke(cr);
}

static void ss_digit_draw(GtkDrawingArea *area, cairo_t *cr, int width,
			  int height, gpointer data)
{
	const struct ss_digit *conf = data;
	int value = conf->value % 10;

	int mask = sevenseg_vals[value];
	double total_seg_len =
		conf->geometry->seg_len + 2 * conf->geometry->seg_wt;

	cairo_move_to(cr, conf->geometry->seg_wt, conf->geometry->seg_wt);
	draw_h_seg(cr, mask & 64, conf);
	cairo_move_to(cr, conf->geometry->seg_wt, conf->geometry->seg_wt);
	draw_v_seg(cr, mask & 32, conf);
	cairo_move_to(cr, conf->geometry->seg_wt + total_seg_len,
		      conf->geometry->seg_wt);
	draw_v_seg(cr, mask & 16, conf);
	cairo_move_to(cr, conf->geometry->seg_wt,
		      conf->geometry->seg_wt + total_seg_len);
	draw_h_seg(cr, mask & 8, conf);
	cairo_move_to(cr, conf->geometry->seg_wt,
		      conf->geometry->seg_wt + total_seg_len);
	draw_v_seg(cr, mask & 4, conf);
	cairo_move_to(cr, conf->geometry->seg_wt + total_seg_len,
		      conf->geometry->seg_wt + total_seg_len);
	draw_v_seg(cr, mask & 2, conf);
	cairo_move_to(cr, conf->geometry->seg_wt,
		      conf->geometry->seg_wt + 2 * total_seg_len);
	draw_h_seg(cr, mask & 1, conf);
}

static void draw_colon_box(cairo_t *cr, const struct ss_colon *conf)
{
	cairo_rel_line_to(cr, conf->geometry->seg_wt, 0);
	cairo_rel_line_to(cr, 0, conf->geometry->seg_wt);
	cairo_rel_line_to(cr, -1 * conf->geometry->seg_wt, 0);
	cairo_close_path(cr);

	cairo_set_line_width(cr, conf->geometry->border_wt);
	cairo_set_source_rgb(cr, conf->color->r, conf->color->g,
			     conf->color->b);
	cairo_fill_preserve(cr);
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_stroke(cr);
}

static void ss_colon_draw(GtkDrawingArea *area, cairo_t *cr, int width,
			  int height, gpointer data)
{
	struct ss_colon *conf = data;

	cairo_move_to(cr, 0,
		      conf->geometry->seg_len - conf->geometry->seg_wt / 2);
	draw_colon_box(cr, conf);
	cairo_move_to(cr, 0,
		      conf->geometry->seg_len + 5.5 * conf->geometry->seg_wt);
	draw_colon_box(cr, conf);
}

static void get_time_info(struct time_info *info)
{
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

static void update_clock_widgets(struct ss_digit *digits[6], int h, int m,
				 int s)
{
	int vals[] = { h / 10, h % 10, m / 10, m % 10, s / 10, s % 10 };

	for (int i = 0; i < 6; i++) {
		if (1 /*digits[i]->value != vals[i]*/) {
			digits[i]->value = vals[i];
			gtk_widget_queue_draw(digits[i]->drawing_area);
		}
	}
}

static const char *utc_offset_to_str(int offset)
{
	switch (offset) {
	case -5:
		return "CENTRAL DAYLIGHT";
	case -6:
		return "CENTRAL STANDARD";
	default:
		return "LOCAL";
	}
}

static gboolean update_clock(gpointer data)
{
	struct clockapp *app = data;
	struct time_info tinfo;

	get_time_info(&tinfo);

	update_clock_widgets(app->utc.digits, tinfo.utc_h, tinfo.utc_m,
			     tinfo.utc_s);
	update_clock_widgets(app->local.digits, tinfo.local_h, tinfo.local_m,
			     tinfo.local_s);

	if (app->prev_utc_offset != tinfo.utc_offset) {
		app->prev_utc_offset = tinfo.utc_offset;

		GString *str = g_string_new("");
		g_string_printf(
			str, "<span size=\"%d\">%s TIME (UTC%c%u)</span>",
			app_config.labels.text_pt,
			utc_offset_to_str(tinfo.utc_offset),
			tinfo.utc_offset < 0 ? '-' : '+',
			tinfo.utc_offset * (tinfo.utc_offset < 0 ? -1 : 1));

		g_string_free(str, true);
	}

	GDateTime *now = g_date_time_new_now_utc();
	int millis = g_date_time_get_microsecond(now) / 1000;
	int to_go = 1000 - millis + 1;
	// 1ms buffer makes sure we are squarely in the next second next time we update
	// nobody will notice

	g_timeout_add(to_go, update_clock, app);

	return FALSE;
}

static struct ss_colon *ss_colon_create(const struct ss_geometry *geometry,
					const struct rgb *color)
{
	struct ss_colon *colon = malloc(sizeof(*colon));

	colon->geometry = geometry;
	colon->color = color;

	colon->drawing_area = gtk_drawing_area_new();
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(colon->drawing_area),
				       ss_colon_draw, colon, NULL);
	gtk_drawing_area_set_content_height(
		GTK_DRAWING_AREA(colon->drawing_area),
		2 * geometry->seg_len + 6 * geometry->seg_wt);
	gtk_drawing_area_set_content_width(
		GTK_DRAWING_AREA(colon->drawing_area), geometry->seg_wt);

	return colon;
}

static struct ss_digit *ss_digit_create(const struct ss_geometry *geometry,
					const struct ss_colors *colors)
{
	struct ss_digit *digit = malloc(sizeof(*digit));

	digit->geometry = geometry;
	digit->colors = colors;
	digit->value = 0;

	digit->drawing_area = gtk_drawing_area_new();
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(digit->drawing_area),
				       ss_digit_draw, digit, NULL);
	gtk_drawing_area_set_content_height(
		GTK_DRAWING_AREA(digit->drawing_area),
		2 * geometry->seg_len + 6 * geometry->seg_wt);
	gtk_drawing_area_set_content_width(
		GTK_DRAWING_AREA(digit->drawing_area),
		geometry->seg_len + 4 * geometry->seg_wt);

	return digit;
}

static GtkWidget *create_formatted_label(const char *text,
					 const struct text_config *config)
{
	GString *str = g_string_new("");
	g_string_printf(str, "<span size=\"%d\">%s</span>", config->text_pt,
			text);

	GtkWidget *label = gtk_label_new(str->str);
	gtk_label_set_use_markup(GTK_LABEL(label), true);

	g_string_free(str, true);

	return label;
}

static void create_clock(GtkWidget *grid, struct ss_clock *clock,
			 const struct ss_geometry *geometry,
			 const struct ss_colors *colors, int row)
{
	for (int i = 0; i < 6; i++)
		clock->digits[i] = ss_digit_create(geometry, colors);
	for (int i = 0; i < 2; i++)
		clock->colons[i] = ss_colon_create(geometry, &colors->on);

	gtk_grid_attach(GTK_GRID(grid), clock->digits[0]->drawing_area, 0, row,
			1, 1);
	gtk_grid_attach(GTK_GRID(grid), clock->digits[1]->drawing_area, 1, row,
			1, 1);

	gtk_grid_attach(GTK_GRID(grid), clock->colons[0]->drawing_area, 2, row,
			1, 1);

	gtk_grid_attach(GTK_GRID(grid), clock->digits[2]->drawing_area, 3, row,
			1, 1);
	gtk_grid_attach(GTK_GRID(grid), clock->digits[3]->drawing_area, 4, row,
			1, 1);

	gtk_grid_attach(GTK_GRID(grid), clock->colons[1]->drawing_area, 5, row,
			1, 1);

	gtk_grid_attach(GTK_GRID(grid), clock->digits[4]->drawing_area, 6, row,
			1, 1);
	gtk_grid_attach(GTK_GRID(grid), clock->digits[5]->drawing_area, 7, row,
			1, 1);
}

static struct clockapp *clockapp_create(const struct config *config, int width,
					int height)
{
	int grid_height;
	struct clockapp *app = malloc(sizeof(*app));
	app->grid = gtk_grid_new();

	app->geometry.seg_wt = config->seg_wt;
	app->geometry.border_wt = config->border_wt;

	app->prev_utc_offset = 25; // force an update of the time offset message

	app->l_utc = create_formatted_label("COORDINATED UNIVERSAL TIME",
					    &config->labels);
	gtk_grid_attach(GTK_GRID(app->grid), app->l_utc, 0, 0, 8, 1);

	app->l_hours = create_formatted_label("HOURS", &config->labels);
	gtk_grid_attach(GTK_GRID(app->grid), app->l_hours, 0, 2, 2, 1);

	app->l_minutes = create_formatted_label("MINUTES", &config->labels);
	gtk_grid_attach(GTK_GRID(app->grid), app->l_minutes, 3, 2, 2, 1);

	app->l_seconds = create_formatted_label("SECONDS", &config->labels);
	gtk_grid_attach(GTK_GRID(app->grid), app->l_seconds, 6, 2, 2, 1);

	app->l_local = create_formatted_label("LOCAL TIME", &config->labels);
	gtk_grid_attach(GTK_GRID(app->grid), app->l_local, 0, 4, 8, 1);

	gtk_grid_set_column_spacing(GTK_GRID(app->grid), app->geometry.seg_wt);

	gtk_widget_measure(app->grid, GTK_ORIENTATION_VERTICAL, -1, NULL,
			   &grid_height, NULL, NULL);

	app->geometry.seg_len =
		(height - grid_height - 12 * app->geometry.seg_wt) / 4;

	create_clock(app->grid, &app->utc, &app->geometry, &app_config.c_utc,
		     1);

	create_clock(app->grid, &app->local, &app->geometry,
		     &app_config.c_local, 3);

	return app;
}

static void on_activate(GtkApplication *app, gpointer user_data)
{
	GtkWidget *window = gtk_application_window_new(app);
	gtk_window_set_title(GTK_WINDOW(window), "utc-clock");
	//gtk_window_set_decorated(GTK_WINDOW(window), false);
	//gtk_window_maximize(GTK_WINDOW(window));
	gtk_window_fullscreen(GTK_WINDOW(window));

	struct clockapp *clock =
		clockapp_create(&app_config, DISPLAY_WIDTH, DISPLAY_HEIGHT);

	GtkWidget *centerbox = gtk_center_box_new();
	gtk_center_box_set_center_widget(GTK_CENTER_BOX(centerbox),
					 clock->grid);
	gtk_window_set_child(GTK_WINDOW(window), centerbox);

	gtk_window_present(GTK_WINDOW(window));

	g_timeout_add(0, update_clock, clock);
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