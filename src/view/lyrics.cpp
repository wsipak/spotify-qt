#include "lyrics.hpp"
#include "mainwindow.hpp"

#include <QVBoxLayout>
#include <QList>
#include <QListWidgetItem>

View::Lyrics::Lyrics(const lib::http_client &httpClient,
	lib::cache &cache, QWidget *parent)
	: QWidget(parent),
	cache(cache),
	lyrics(httpClient)
{
	auto *layout = new QVBoxLayout(this);

	status = new QLabel(this);
	layout->addWidget(status);

	lyricsList = new QListWidget(this);
	lyricsList->setWordWrap(true);
	lyricsList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	layout->addWidget(lyricsList);
}

void View::Lyrics::open(const lib::spt::track &track)
{
//	const auto &cached = cache.get_track_info(track);
//	if (cached.is_valid())
//	{
//		setPlainText(QString::fromStdString(cached.lyrics));
//		return;
//	}

	status->setText(QStringLiteral("Searching..."));

	lyrics.search(track, [this](const lib::result<std::vector<lib::lrc::search_result>> &result)
	{
		if (!result.success() || result.value().empty())
		{
			status->setText(result.message().empty()
				? QStringLiteral("No results")
				: QString::fromStdString(result.message()));

			return;
		}

		const auto lyricsId = result.value().front().lyrics_id;
		lyrics.lyrics(lyricsId, [this](const lib::result<lib::lrc::lyrics> &result)
		{
			if (!result.success())
			{
				status->setText(QString::fromStdString(result.message()));
				return;
			}

			load(result.value());
		});
	});
}

void View::Lyrics::load(const lib::lrc::lyrics &loaded)
{
	lyricsList->clear();
	if (loaded.lines.empty())
	{
		return;
	}

	for (const auto &line: loaded.lines)
	{
		auto *item = new QListWidgetItem(lyricsList);
		item->setText(QString::fromStdString(line.text));
		item->setData(timestampRole, (qlonglong) line.timestamp);
	}

	auto *window = MainWindow::find(parentWidget());
	if (window == nullptr)
	{
		return;
	}

	MainWindow::connect(window, &MainWindow::tick, this, &View::Lyrics::onTick);
}

auto View::Lyrics::getTimestamp(const QListWidgetItem *item) -> qlonglong
{
	return item->data(timestampRole).toLongLong();
}

void View::Lyrics::onTick(const lib::spt::playback &playback)
{
	if (!playback.is_playing)
	{
		return;
	}

	if (playback.item.id != currentTrack.id)
	{
		lyricsList->setCurrentItem(nullptr);
		return;
	}

	auto *currentItem = lyricsList->currentItem();
	QListWidgetItem *item;
	int index;

	if (currentItem == nullptr)
	{
		index = 0;
		item = lyricsList->item(index);
	}
	else
	{
		index = lyricsList->currentRow();
		item = currentItem;
	}

	if (getTimestamp(item) < playback.progress_ms)
	{
		QListWidgetItem *next = lyricsList->item(++index);
		while (next != nullptr)
		{
			const auto nextTimestamp = getTimestamp(next);
			if (nextTimestamp > playback.progress_ms)
			{
				break;
			}

			item = next;
			next = lyricsList->item(++index);
		}
	}
	else
	{
		QListWidgetItem *previous = lyricsList->item(--index);
		while (previous != nullptr)
		{
			const auto previousTimestamp = getTimestamp(previous);
			if (previousTimestamp < playback.progress_ms)
			{
				break;
			}

			item = previous;
			previous = lyricsList->item(--index);
		}
	}

	lyricsList->setCurrentItem(item);
	emit lyricsList->scrollToItem(item, QAbstractItemView::PositionAtCenter);
}
